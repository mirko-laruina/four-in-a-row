  /**
 * @file socket_wrapper.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of socket_wrapper.h
 * 
 * @see socket_wrapper.h
 */

#include "logging.h"
#include "network/socket_wrapper.h"
#include "inet_utils.h"

SocketWrapper::SocketWrapper() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0){
        LOG(LOG_ERR, "Could not create socket!\n");
        perror("Error: ");
        return;    
    }
}

Message* SocketWrapper::receiveAnyMsg(size_t size){
    int len;
    char* in_buffer;

    in_buffer = (char*) malloc(size);

    len = recv(socket_fd, in_buffer, size, 0);

    Message *m = readMessage(in_buffer, len);

    free(in_buffer);

    return m;
}

Message* SocketWrapper::receiveMsg(MessageType type, size_t size /*=MAX_MSG_SIZE*/){
    return this->receiveMsg(&type, 1, size);
}

Message* SocketWrapper::receiveMsg(MessageType type[], int n_types, 
                                size_t size /*=MAX_MSG_SIZE*/){
    Message *m = NULL;
    while (m == NULL){
        m = this->receiveAnyMsg();
        if (m != NULL){
            for (int i = 0; i < n_types; i++){
                if (m->getType() == type[i]){
                    return m;
                }
                LOG(LOG_WARN, "Received unexpected message of type %d",  m->getType());
            }
        }
    }
    //TODO: add timeout?
    return NULL;
}


int SocketWrapper::sendMsg(Message *msg){
    int msglen, len;
    char *out_buffer;

    msglen = msg->size();
    out_buffer = (char*) malloc(msglen);

    msg->write(out_buffer);

    len = send(socket_fd, out_buffer, msglen, 0);
    if (len != msglen){
        LOG(LOG_ERR, "Error sending %s: len (%d) != msglen (%d)", 
            msg->getName().c_str(),
            len, 
            msglen
        );
        return 1;
    }

    LOG(LOG_DEBUG, "Sent message %s", msg->getName().c_str());
    
    free(out_buffer);
    return 0;
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
