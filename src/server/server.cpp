/**
 * @file server.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of a 4-in-a-row online server
 * 
 * The select implementation was inspired by
 * https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 *
 * @date 2020-05-23
 */
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <map>
#include <queue>
#include <utility>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "logging.h"
#include "config.h"
#include "network/socket_wrapper.h"
#include "network/host.h"

#include "user.h"
#include "user_list.h"
#include "utils/message_queue.h"

#include "security/crypto_utils.h"

using namespace std;

typedef pair<int,Message*> msgqueue_t;
typedef map<string,X509*> cert_map_t;

static UserList user_list;
static MessageQueue<msgqueue_t,MAX_QUEUE_LENGTH> message_queue;
static pthread_t threads[N_THREADS];
static cert_map_t cert_map;
static X509* cert;

void logUnexpectedMessage(User* u, Message* m){
    LOG(LOG_WARN, "User %s (state %d) was not expecting a message of type %d", 
        u->getUsername().c_str(), (int)u->getState(), (int)m->getType());
}

void doubleLock(User* u_with_lock, User* u_without_lock){
    // prevent deadlocks
    if (u_without_lock->getUsername() < u_with_lock->getUsername()){
        u_with_lock->unlock();
        u_without_lock->lock();
        u_with_lock->lock();
    } else{
        u_without_lock->lock();
    }
}

void doubleUnlock(User* u_keep_lock, User* u_unlock){
    // prevent deadlocks
    if (u_unlock->getUsername() < u_keep_lock->getUsername()){
        u_keep_lock->unlock();
        u_unlock->unlock();
        u_keep_lock->lock();
    } else{
        u_unlock->unlock();
    }
}

bool handleRegisterMessage(User* u, RegisterMessage* msg){
    string username = msg->getUsername();
    u->setUsername(username);

    if (!user_list.exists(username)){
        u->setState(AVAILABLE); 
        // readd with username
        user_list.add(u);
        return true;
    } else {
        LOG(LOG_WARN, "User %s already registered!", username.c_str());
        //TODO send error
        u->setState(DISCONNECTED); 
        return false;
    }
}

bool handleChallengeMessage(User* u, ChallengeMessage* msg){
    bool res;
    string chlg_username = msg->getUsername();
    User* challenged = user_list.get(chlg_username);
    if (challenged == NULL || challenged == u){
        GameCancelMessage cancel_msg(chlg_username);
        return u->getSocketWrapper()->sendMsg(&cancel_msg) == 0;
    } 

    doubleLock(u, challenged);

    if (u->getState() != AVAILABLE){
        // someother thing concurrently happened, ignore
        doubleUnlock(u, challenged);
        user_list.yield(challenged);
        return true;
    }

    if (challenged->getState() != AVAILABLE){
        // someother thing concurrently happened, abort
        GameCancelMessage cancel_msg(chlg_username);
        res = u->getSocketWrapper()->sendMsg(&cancel_msg) == 0;

        doubleUnlock(u, challenged);
        user_list.yield(challenged);
        return res;
    }

    // both are available, send challenge
    ChallengeForwardMessage fwd_msg(u->getUsername());
    if (challenged->getSocketWrapper()->sendMsg(&fwd_msg) == 0){
        // challenge sent, mark them as playing until I receive a response
        u->setState(CHALLENGED);
        u->setOpponent(challenged->getUsername());
        challenged->setState(CHALLENGED);
        challenged->setOpponent(u->getUsername());
        res = true;
    } else{
        // connection error -> assume disconnected and notify u
        GameCancelMessage cancel_msg(chlg_username);
        challenged->setState(DISCONNECTED);
        res = u->getSocketWrapper()->sendMsg(&cancel_msg) == 0;
    }

    doubleUnlock(u, challenged);
    user_list.yield(challenged);

    return res;
}

bool handleGameEndMessage(User* u, GameEndMessage* msg){
    u->setState(AVAILABLE);
    return true;
}

bool handleUsersListRequestMessage(User* u, UsersListRequestMessage* msg){
    UsersListMessage ul_msg(user_list.listAvailableFromTo(msg->getOffset()));
    return u->getSocketWrapper()->sendMsg(&ul_msg) == 0;
}

bool handleChallengeResponseMessage(User* u, ChallengeResponseMessage* msg){
    bool res;
    User *opponent = user_list.get(u->getOpponent());
    if (opponent == NULL || opponent == u){
        // opponent disconnected or invalid opponent -> cancel
        u->setState(AVAILABLE);
        GameCancelMessage cancel_msg(u->getOpponent());
        return u->getSocketWrapper()->sendMsg(&cancel_msg) == 0;
    }

    doubleLock(u, opponent);

    if (msg->getResponse()){ // accepted
        if (u->getState() != CHALLENGED){
            // someother thing concurrently happened, abort
            // maybe this refers to old challenge
            doubleUnlock(u, opponent);
            user_list.yield(opponent);
            return true;
        }

        if (opponent->getState() != CHALLENGED){
            // opponent is in wrong state...
            // maybe this refers to old challenge
            // notify u of opponent not ready
            GameCancelMessage cancel_msg(opponent->getUsername());
            res = u->getSocketWrapper()->sendMsg(&cancel_msg) == 0;
            doubleUnlock(u, opponent);
            user_list.yield(opponent);
            return res;
        }

        struct sockaddr_in opp_addr = opponent->getSocketWrapper()      
                                        ->getConnectedHost().getAddress();
        opp_addr.sin_port = 0;
        cert_map_t::iterator opp_pair = cert_map.find(opponent->getUsername());
        if(opp_pair == cert_map.end()) {
            doubleUnlock(u, opponent);
            user_list.yield(opponent);
            return false;
        }
        GameStartMessage msg_to_u(opponent->getUsername(), opp_addr, opp_pair->second);

        struct sockaddr_in u_addr = u->getSocketWrapper()      
                                        ->getConnectedHost().getAddress();
        u_addr.sin_port = htons(msg->getListenPort());
        cert_map_t::iterator u_pair = cert_map.find(u->getUsername());
        if(u_pair == cert_map.end()) {
            doubleUnlock(u, opponent);
            user_list.yield(opponent);
            return false;
        }
        GameStartMessage msg_to_opp(u->getUsername(), u_addr, u_pair->second);

        int res_u = u->getSocketWrapper()->sendMsg(&msg_to_u);
        int res_opp = opponent->getSocketWrapper()->sendMsg(&msg_to_opp);

        if (res_u == 0 && res_opp == 0){
            //success
            u->setState(PLAYING);
            opponent->setState(PLAYING);

            res = true;
        } else if (res_u != 0 && res_opp != 0){
            // both disconnected
            opponent->setState(DISCONNECTED);
            u->setState(DISCONNECTED);
            res = false;
        } else {
            if (res_u != 0){ // just u disconnected => notify opp
                GameCancelMessage cancel_msg(u->getUsername());
                if (opponent->getSocketWrapper()->sendMsg(&cancel_msg) == 0){
                    opponent->setState(AVAILABLE);
                } else {
                    opponent->setState(DISCONNECTED);
                }
            } else if (res_opp != 0){ // just opp disconnected => notify u
                GameCancelMessage cancel_msg(opponent->getUsername());
                if (u->getSocketWrapper()->sendMsg(&cancel_msg) == 0){
                    u->setState(AVAILABLE);
                    res = true;
                } else {
                    u->setState(DISCONNECTED);
                    res = false;
                }
            }
        }
    } else{ // rejected
        u->setState(AVAILABLE);
        GameCancelMessage cancel_msg(u->getUsername());
        if (opponent->getSocketWrapper()->sendMsg(&cancel_msg) == 0){
            opponent->setState(AVAILABLE);
        } else{
            opponent->setState(DISCONNECTED);
        }
    }

    doubleUnlock(u, opponent);
    user_list.yield(opponent);

    return res;
}

bool handleClientHelloMessage(User* u, ClientHelloMessage* chm){
    string username = chm->getMyId();
    cert_map_t::iterator res;
    SecureSocketWrapper *sw = u->getSocketWrapper();
    
    if ((res = cert_map.find(username)) != cert_map.end()){
        sw->setOtherCert(res->second);
        int ret = u->getSocketWrapper()->handleClientHello(chm);
        return ret == 0;
    } else{
        LOG(LOG_WARN, "User %s not found in cert_map", username.c_str());
        return false;
    }
}

bool handleClientVerifyMessage(User* u, ClientVerifyMessage* cvm){
    int ret = u->getSocketWrapper()->handleClientVerify(cvm);
    if(ret == 0){
        u->setState(SECURELY_CONNECTED);
        return true;
    } else {
        LOG(LOG_ERR, "Client Verify failed!");
        u->setState(DISCONNECTED);
        return false;
    }
}

bool handleCertificateRequestMessage(User* u, CertificateRequestMessage* crm){
    CertificateMessage cm(cert);
    int ret = u->getSocketWrapper()->sendMsg(&cm);
    if(ret == 0){
        return true;
    } else {
        LOG(LOG_ERR, "Error sending certificate to client!");
        u->setState(DISCONNECTED);
        return false;
    }
}

bool handleMessage(User* user, Message* raw_msg){
    bool res = true;

    user->lock();

    try{

        Message* msg = user->getSocketWrapper()->handleMsg(raw_msg);

        LOG(LOG_INFO, "User %s (state %d) received a message of type %s",
            user->getUsername().c_str(), (int) user->getState(), msg->getName().c_str());

        switch(user->getState()){
            case JUST_CONNECTED:
                switch(msg->getType()){
                    case CLIENT_HELLO:
                        res = handleClientHelloMessage(user,
                            dynamic_cast<ClientHelloMessage*>(msg));
                        break;
                    case CLIENT_VERIFY:
                        res = handleClientVerifyMessage(user,
                            dynamic_cast<ClientVerifyMessage*>(msg));
                        break;
                    case CERT_REQ:
                        res = handleCertificateRequestMessage(user,
                            dynamic_cast<CertificateRequestMessage*>(msg));
                    // TODO: handle cert request
                    default:
                        logUnexpectedMessage(user, msg);
                }
                break;
            case SECURELY_CONNECTED:
                switch(msg->getType()){
                    case REGISTER:
                        res = handleRegisterMessage(user,        
                            dynamic_cast<RegisterMessage*>(msg));
                        break;
                    default:
                        logUnexpectedMessage(user, msg);
                }
                break;
            case AVAILABLE:
                switch(msg->getType()){
                    case CHALLENGE:
                        res = handleChallengeMessage(user,        
                            dynamic_cast<ChallengeMessage*>(msg));
                        break;
                    case USERS_LIST_REQ:
                        res = handleUsersListRequestMessage(user,
                            dynamic_cast<UsersListRequestMessage*>(msg));
                        break;
                    default:
                        logUnexpectedMessage(user, msg);
                }
                break;
            case CHALLENGED: 
                switch(msg->getType()){
                    case CHALLENGE_RESP:
                        res = handleChallengeResponseMessage(user,
                            dynamic_cast<ChallengeResponseMessage*>(msg));
                        break;
                    default:
                        logUnexpectedMessage(user, msg);
                }
                break;
            case PLAYING: 
                switch(msg->getType()){
                    case GAME_END:
                        res = handleGameEndMessage(user,
                            dynamic_cast<GameEndMessage*>(msg));
                        break;
                    default:
                        logUnexpectedMessage(user, msg);
                }
                break;
            default:
                LOG(LOG_ERR, "User %s is in unrecognized state %d", 
                    user->getUsername().c_str(), (int) user->getState());
        }

        delete msg;
    } catch(const char* error_msg){
        LOG(LOG_ERR, "Caught error: %s", error_msg);
        res = false;
    }

    user->unlock();
    return res;
}

void* worker(void *args){
    while (1){
        msgqueue_t p = message_queue.pullWait();
        User* u = user_list.get(p.first);
        if (u != NULL){
            if (!handleMessage(u, p.second)){
                // Connection error -> assume disconnected
                u->setState(DISCONNECTED);
            }
            user_list.yield(u);
        }
    }
        
}

void init_threads(){
    for (int i=0; i < N_THREADS; i++){
        pthread_create(&threads[i], NULL, worker, NULL);
    }
}

bool checkCertsInCertMap(X509_STORE* store, cert_map_t cert_map){
    for (cert_map_t::iterator it = cert_map.begin();
        it != cert_map.end();
        ++it
    ){
        if (!verify_peer_cert(store, it->second)){
            LOG(LOG_ERR, "Validation failed for certificate in directory: %s", 
                    it->first.c_str());
            return false;
        }
    }
    return true;

}

int main(int argc, char** argv){
    fd_set active_fd_set, read_fd_set;

    if (argc < 7){
        cout<<"Usage: "<<argv[0]<<" port cert.pem key.pem cacert.pem crl.pem"<<endl;
        exit(1);
    }

    int port = atoi(argv[1]);
    cert = load_cert_file(argv[2]);
    EVP_PKEY* key = load_key_file(argv[3], NULL);
    X509* cacert = load_cert_file(argv[4]);
    X509_CRL* crl = load_crl_file(argv[5]);
    X509_STORE* store = build_store(cacert, crl);
    cert_map = buildCertMapFromDirectory(argv[6]);

    if (cert_map.size() == 0){
        LOG(LOG_ERR, "No certificates found in directory");
        return 1;
    }

    if (!checkCertsInCertMap(store, cert_map)){
        return 1;
    }

    LOG(LOG_INFO, "Loaded certificates from %s", argv[6]);

    ServerSecureSocketWrapper server_sw(cert, key, store);

    int ret = server_sw.bindPort(port);
    if (ret != 0){
        LOG(LOG_FATAL, "Error binding to port %d", port);
        exit(1);
    }

    LOG(LOG_INFO, "Binded to port %d", port);

    init_threads();

    LOG(LOG_INFO, "Started %d worker threads", N_THREADS);

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(server_sw.getDescriptor(), &active_fd_set);

    LOG(LOG_INFO, "Polling open sockets");

    while (1){
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            if (errno == EBADF){ // clean closed sockets
                LOG(LOG_DEBUG, "Bad file descriptor");
                for (int i = 0; i < FD_SETSIZE; ++i){
                    if (FD_ISSET(i, &active_fd_set)){
                        if (i != server_sw.getDescriptor() 
                                && !user_list.exists(i)
                            ){
                            // user was disconnected but I still need to clear it
                            LOG(LOG_DEBUG, "Cleared fd %d", i);
                            FD_CLR(i, &active_fd_set);
                        }
                    }
                }
                continue;
            }

            LOG_PERROR(LOG_FATAL, "Error in select: %s");
            exit(1);
        }

        /* Service all the sockets with input pending. */
        for (int i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET(i, &read_fd_set)){
                if (i == server_sw.getDescriptor()) {
                    /* Connection request on original socket. */
                    SecureSocketWrapper* sw = server_sw.acceptClient();

                    LOG(LOG_INFO, "New connection from %s", 
                        sw->getConnectedHost().toString().c_str());

                    FD_SET(sw->getDescriptor(), &active_fd_set);

                    User *u = new User(sw);
                    user_list.add(u);
                } else {
                    User *u = user_list.get(i);
                    if (u->getState() == DISCONNECTED){
                        LOG(LOG_DEBUG, "Received message from disconnected user with countRefs = %d", u->countRefs());
                        user_list.yield(u);
                        FD_CLR(i, &active_fd_set); // ignore him
                        continue;
                    }
                    const char* u_addr_str = u->getSocketWrapper()
                            ->getConnectedHost().toString().c_str();
                    LOG(LOG_INFO, "Available message from %s (%s)",
                        u->getUsername().c_str(), u_addr_str);
                    try{
                        Message* m = u->getSocketWrapper()->readPartMsg();
                        if (m != NULL)
                            message_queue.pushSignal(msgqueue_t(i, m));
                    } catch(const char* msg){
                        LOG(LOG_WARN, "Client %s disconnected: %s", 
                            u_addr_str, msg);
                        u->setState(DISCONNECTED);
                    }
                    user_list.yield(u);
                    if (!user_list.exists(i)){
                        // user was disconnected -> clear it
                        FD_CLR(i, &active_fd_set);
                    }
                }
            } else if (FD_ISSET(i, &active_fd_set)){
                if (i != server_sw.getDescriptor() 
                                && !user_list.exists(i)
                ){
                    LOG(LOG_DEBUG, "Cleared fd %d", i);
                    // user was disconnected but I still need to clear it
                    FD_CLR(i, &active_fd_set);
                }
            }
        }
    }
}


