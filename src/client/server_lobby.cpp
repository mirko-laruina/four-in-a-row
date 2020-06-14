/**
 * @file server_lobby.cpp
 * @author Riccardo Mancini
 *
 * @brief Implementation of the function that handles user and network input 
 *          while the user is in the server lobby waiting for a game to start
 * 
 * The select implementation was inspired by
 * https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 * 
 * @date 2020-05-29
 */

#include <sys/select.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <sstream>

#include "utils/args.h"
#include "security/secure_socket_wrapper.h"
#include "security/crypto_utils.h"

#include "server_lobby.h"
#include "server.h"

using namespace std;

/** stdin has file descriptor 0 in Unix */
#define STDIN (0)

void printAvailableActions(){
    cout<<"You can list users, challenge a user, exit or simply wait for other users to challenge you."<< endl;
    cout<<"To list users type: `list`"<< endl;
    cout<<"To challenge a user type: `challenge username`"<< endl;
    cout<<"To exit type: `exit`"<< endl;
    cout<<"NB: you cannot receive challenges if you are challenging another user"<< endl;
}

int doAction(Args args, Server *server, SecureHost* peer_host){
    ostringstream os;
    os << args;
    LOG(LOG_DEBUG, "Args: %s", os.str().c_str());
    if (args.getArgc() == 1 && strcmp(args.getArgv(0), "exit") == 0){
        return -2;
    } else if (args.getArgc() == 1 && strcmp(args.getArgv(0), "list") == 0){
        cout<<"Retrieving the list of users..."<<endl;
        string userlist = server->getUserList();
        if (userlist.empty()){
            return 1;
        }
        cout<<"Online users: "<<userlist<<endl;
        return 0;
    } else if (args.getArgc() == 2 && strcmp(args.getArgv(0), "challenge") == 0){
        cout<<"Sending challenge to "<<args.getArgv(1)<<" and waiting for response..."<<endl;
        string username(args.getArgv(1));
        int ret = server->challengePeer(username, peer_host);
        switch (ret){ 
            case -1: // refused
                cout<<username<<" refused your challenge"<<endl;
                return 0;
            case 0: //accepted
                cout<<username<<" accepted your challenge"<<endl;
                return -1;
            default:
                cout<<"Error connecting to server!"<<endl;
                return 1;
        } 
    } else if (args.getArgc() < 0) {
        return -2; //exit
    } else {
        return 0;
    }

}

int handleReceivedChallenge(Server *server, 
                            ChallengeForwardMessage* msg, 
                            SecureHost* peer_host,
                            uint16_t* listen_port){
    cout<<endl<<"You received a challenge from "<<msg->getUsername()<<endl;
    cout<<"Do you want to accept? (y/n)";

    bool response;

    do{
        cout<<"> "<<flush;
        Args args(cin);
        if (args.getArgc() == 1 && strcmp(args.getArgv(0), "y")){
            response = true;
            break;
        } else if (args.getArgc() == 1 && strcmp(args.getArgv(0), "n")){
            response = false;
            break;
        } else if (args.getArgc() < 0){ // EOF
            response = false;
            break;
        } else{
            continue;
        }
    } while(1);


    return server->replyPeerChallenge(msg->getUsername(), response, peer_host, listen_port);
}

ConnectionMode handleMessage(Message* msg, Server* server){
    ChallengeForwardMessage* cfm;
    SecureHost peer_host;
    uint16_t listen_port;

    int ret;
    LOG(LOG_INFO, "Server sent message %s", msg->getName().c_str());
    switch(msg->getType()){
        case CHALLENGE_FWD:
            cfm = dynamic_cast<ChallengeForwardMessage*>(msg);
            ret = handleReceivedChallenge(server, cfm, &peer_host, &listen_port);
            switch (ret){
                case -1: // game canceled
                    cout<<"Game was canceled"<<endl;
                    return ConnectionMode(CONTINUE);
                case 0:
                    cout<<"Starting game..."<<endl;
                    return ConnectionMode(WAIT_FOR_PEER, peer_host, listen_port);
                default:
                    cout<<"Error"<<endl; 
                    return ConnectionMode(EXIT, CONNECTION_ERROR);
            }
            break;
        default:
            // other messages are handled internally to 
            // Server since they require the user to wait
            LOG(LOG_WARN, "Received unexpected message %s", msg->getName().c_str());
            return ConnectionMode(CONTINUE);
    }
}

ConnectionMode handleStdin(Server* server){
    SecureHost peer_host;

    int ret;
    // Input from user
    Args args(cin);
    if (args.getArgc() < 0){
        ret = -2; // received EOF
    } else{
        ret = doAction(args, server, &peer_host);
    }
    switch (ret){
        case 0: // do nothing
            LOG(LOG_DEBUG, "No action");
            return ConnectionMode(CONTINUE);
        case 1: // error
            cout<<"Error!"<<endl;
            return ConnectionMode(EXIT, CONNECTION_ERROR);
        case -1: // challenge accepted
            cout<<"Starting game..."<<endl;
            return ConnectionMode(CONNECT_TO_PEER, peer_host, 0);
        case -2:
            cout<<"Bye"<<endl;
            return ConnectionMode(EXIT, OK);
    default:
        return ConnectionMode(CONTINUE);
    }
}


ConnectionMode serverLobby(Server* server){
    fd_set active_fd_set, read_fd_set;

    string username = server->getPlayerUsername();

    if (!server->isConnected()){
        cout<<"Registering to "<<server->getHost().toString()<<" as "<<username<<endl;
        if (server->registerToServer() != 0){
            cout<<"Connection to "<<server->getHost().toString()<<" failed!"<<endl;
            return ConnectionMode(EXIT, CONNECTION_ERROR);
        }
        LOG(LOG_INFO, "Server %s is now connected", 
                        server->getHost().toString().c_str());
    } else {
        LOG(LOG_INFO, "Server %s was already connected", 
                        server->getHost().toString().c_str());
    }

    SecureHost peer_host;

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(server->getSocketWrapper()->getDescriptor(), &active_fd_set);
    FD_SET(STDIN, &active_fd_set);

    printAvailableActions();

    while (1){
        cout<<endl<<"> "<<flush;

        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            LOG_PERROR(LOG_ERR, "Error in select: %s");
            return ConnectionMode(EXIT, GENERIC_ERROR);
        }

        /* Service all the socketsexit(1); with input pending. */
        for (int i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET(i, &read_fd_set)){
                if (i == server->getSocketWrapper()->getDescriptor()){
                    // Message from server.
                    Message* msg;
                    try{
                        msg = server->getSocketWrapper()->receiveAnyMsg();
                    } catch(const char* msg){
                        LOG(LOG_ERR, "Error: %s", msg);
                        return ConnectionMode(EXIT, CONNECTION_ERROR);
                    }

                    ConnectionMode m = handleMessage(msg, server);
                    if (m.connection_type != CONTINUE){
                        return m;
                    }
                } else if (i == STDIN){
                    ConnectionMode m = handleStdin(server);
                    if (m.connection_type != CONTINUE){
                        return m;
                    }
                }
            }
        }
    }
}

