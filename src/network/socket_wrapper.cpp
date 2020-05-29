  /**
 * @file socket_wrapper.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of socket_wrapper.h
 * 
 * @see socket_wrapper.h
 */

#include <assert.h>
#include "logging.h"
#include "network/socket_wrapper.h"
#include "utils/dump_buffer.h"
#include "network/inet_utils.h"

SocketWrapper::SocketWrapper() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0){
        LOG(LOG_ERR, "Could not create socket!\n");
        perror("Error: ");
        return;    
    }
    buf_idx = 0;
}
Message* SocketWrapper::readPartMsg(){
    int len;
    msglen_t msglen;

    if (buf_idx < sizeof(msglen)){ // I first need to read msglen
        // read available message
        len = read(socket_fd, buffer+buf_idx, sizeof(msglen)-buf_idx);
        // I will read rest of it in another moment since I do not know
        // whether other data is available.
        // TODO: find a way to tell whether socket has other data
    } else{
        // read msg length
        msglen = MSGLEN_NTOH(*((msglen_t*)buffer));
        assert(msglen <= MAX_MSG_SIZE); // must not happen

        // read up to msg length
        len = read(socket_fd, buffer+buf_idx, msglen-buf_idx);
    }

    DUMP_BUFFER_HEX_DEBUG(buffer, len);
    
    if (len < 0){
        perror("Error:");
        throw "Error";
    } else if (len == 0){
        throw "Connection lost";
    } 

    // buf_idx is also the number of read bytes up to now    
    buf_idx += len;

    if (buf_idx < sizeof(msglen)){
        LOG(LOG_DEBUG, "Too few bytes recevied from socket: %d < %lu", 
            buf_idx, sizeof(msglen));
        return NULL;
    }

    // read msg length
    msglen = MSGLEN_NTOH(*((msglen_t*)buffer));

    if (msglen > MAX_MSG_SIZE){
        throw("Message is too big");
    } 

    if (buf_idx != msglen){
        LOG(LOG_DEBUG, "Too few bytes received from socket: %d < %d", 
            buf_idx, msglen);
        return NULL;
    }

    Message *m = readMessage(buffer+sizeof(msglen), msglen-sizeof(msglen));

    // reset buffer
    buf_idx = 0;

    return m;
}

Message* SocketWrapper::receiveAnyMsg(){
    int len;
    msglen_t msglen;

    // read msg length
    len = recv(socket_fd, buffer, sizeof(msglen), MSG_WAITALL);

    DUMP_BUFFER_HEX_DEBUG(buffer, len);
    
    if (len == 0){
        throw "Connection lost";
    } else if (len != sizeof(msglen)){
        LOG(LOG_ERR, "Too few bytes recevied from socket: %d < %lu", 
            len, sizeof(msglen));
        return NULL;
    }
    
    // read msg payload
    msglen = MSGLEN_NTOH(*((msglen_t*)buffer));

    if (msglen > MAX_MSG_SIZE){
        throw("Message is too big");
    } 

    len += recv(socket_fd, buffer+len, msglen-len, MSG_WAITALL);

    DUMP_BUFFER_HEX_DEBUG(buffer, len);
    
    if (len == 0){
        throw "Received EOF";
    } else if (len != msglen){
        LOG(LOG_ERR, "Too few bytes recevied from socket: %d < %d", 
            len, msglen);
        return NULL;
    }

    Message *m = readMessage(buffer+sizeof(msglen), msglen-sizeof(msglen));

    return m;
}

Message* SocketWrapper::receiveMsg(MessageType type){
    return this->receiveMsg(&type, 1);
}

Message* SocketWrapper::receiveMsg(MessageType type[], int n_types){
    Message *m = NULL;
    while (m == NULL){
        try{
            m = this->receiveAnyMsg();
        } catch(const char* msg){
            LOG(LOG_ERR, "%s", msg);
            return NULL;
        }
        if (m != NULL){
            for (int i = 0; i < n_types; i++){
                if (m->getType() == type[i]){
                    return m;
                }
                LOG(LOG_WARN, "Received unexpected message of type %s",  m->getName().c_str());
            }
        }
    }
    //TODO: add timeout?
    return NULL;
}


int SocketWrapper::sendMsg(Message *msg){
    msglen_t msglen, pktlen;
    int len;

    msglen = msg->size();
    pktlen = msglen + sizeof(msglen);
    if (pktlen > MAX_MSG_SIZE){
        LOG(LOG_ERR, "Error sending %s: message is too big (requested: %d, max: %lu)", 
            msg->getName().c_str(),
            msglen, 
            MAX_MSG_SIZE-sizeof(msglen)
        );
        return 1;
    }

    *((msglen_t*)buffer) = MSGLEN_HTON(pktlen);
    msg->write(buffer+sizeof(msglen));

    LOG(LOG_DEBUG, "Sending %s", msg->getName().c_str());

    DUMP_BUFFER_HEX_DEBUG(buffer, pktlen);

    len = send(socket_fd, buffer, pktlen, 0);
    if (len != pktlen){
        LOG(LOG_ERR, "Error sending %s: len (%d) != msglen (%d)", 
            msg->getName().c_str(),
            len, 
            msglen
        );
        return 1;
    }

    LOG(LOG_DEBUG, "Sent message %s", msg->getName().c_str());
    
    return 0;
}

void SocketWrapper::closeSocket(){
    close(socket_fd);
}

int ClientSocketWrapper::connectServer(Host host){
    int ret;

    other_addr = host.getAddress();

    ret = connect(
        socket_fd, 
        (struct sockaddr*) &other_addr, 
        sizeof(other_addr)
    );

    if (ret != 0){
        LOG(LOG_ERR, "Error connecting to %s", 
            sockaddr_in_to_string(host.getAddress()).c_str());
        perror("Error: ");
        return ret;
    }
    
    return ret;
}

ServerSocketWrapper::ServerSocketWrapper(){
    my_addr = make_my_sockaddr_in(0);
    int ret = bind_random_port(socket_fd, &my_addr);
    if (ret <= 0){
        LOG(LOG_ERR, "Error in binding\n");
        perror("Error: ");        
    }

    ret = listen(socket_fd, 10);
    if (ret != 0){
        LOG(LOG_ERR, "Error in setting socket to listen mode\n");
        perror("Error: ");        
    }
}

ServerSocketWrapper::ServerSocketWrapper(int port){
    my_addr = make_my_sockaddr_in(port);
    int ret = bind(socket_fd, (struct sockaddr*) &my_addr, sizeof(my_addr));
    if (ret != 0){
        LOG(LOG_ERR, "Error in binding\n");
        perror("Error: ");        
    }

    ret = listen(socket_fd, 10);
    if (ret != 0){
        LOG(LOG_ERR, "Error in setting socket to listen mode\n");
        perror("Error: ");        
    }
}

SocketWrapper* ServerSocketWrapper::acceptClient(){
    socklen_t len = sizeof(other_addr);
    int new_sd = accept(
        socket_fd, 
        (struct sockaddr*) &other_addr,
        &len
    );

    SocketWrapper *sw = new SocketWrapper(new_sd);
    sw->setOtherAddr(other_addr);
    return sw;
}
