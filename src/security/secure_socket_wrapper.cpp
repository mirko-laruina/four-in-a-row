#include "security/secure_socket_wrapper.h"

SecureSocketWrapper::SecureSocketWrapper() : SocketWrapper(){
    sym_key = "test";
    iv = "test2";
}


SecureSocketWrapper::SecureSocketWrapper(int sd) : SocketWrapper(sd) {
    sym_key = "test";
    iv = "test2";
}

Message* SecureSocketWrapper::decryptMsg(SecureMessage* sm){
    msglen_t sm_len = sm->size();
    LOG(LOG_DEBUG, "Received SecureMessage of size %d", sm_len);
    char* buffer = (char*) malloc(sm_len);
    int buf_len = sm->write(buffer);

    unsigned char* pt = (unsigned char*) malloc(buf_len-1);
    unsigned char* tag = (unsigned char*) malloc(256);

    int ret = aes_gcm_decrypt((unsigned char*) &buffer[1], buf_len-1, NULL, 0,
                              (unsigned char*) sym_key, (unsigned char*) iv,
                                pt, tag);
    LOG(LOG_DEBUG, "Decrypted message of size %d", ret);
        
    if(ret <= 0){
        LOG(LOG_ERR, "Could not decrypt the message");
    }

    Message* m = readMessage((char*) pt, buf_len-1);
    free(buffer);
    free(pt);
    free(tag);
    return m;
}

SecureMessage* SecureSocketWrapper::encryptMsg(Message* m){
    msglen_t m_len = m->size();
    LOG(LOG_DEBUG, "Encrypting message of size %d", m_len);
    char* buffer = (char*) malloc(m_len);
    int buf_len = m->write(buffer);
    
    unsigned char* ct = (unsigned char*) malloc(buf_len+1);
    unsigned char* tag = (unsigned char*) malloc(256);
    ct[0] = (MessageType) SECURE_MESSAGE;
    
    int ret = aes_gcm_encrypt((unsigned char*) buffer, buf_len, NULL, 0,
                                (unsigned char*) sym_key, (unsigned char*) iv,
                                &ct[1], tag);

    LOG(LOG_DEBUG, "Message encrypted %d bytes", ret);
    LOG(LOG_DEBUG, "SecureMessage of size %d", buf_len+1);
    
    if(ret <= 0){
        LOG(LOG_ERR, "Could not encrypt the message");
    }

    SecureMessage* sm = new SecureMessage();
    sm->read((char*) ct, buf_len+1);
    free(ct);
    free(buffer);
    free(tag);
    return sm;
}

Message* SecureSocketWrapper::readPartMsg(){
    SecureMessage* m = (SecureMessage*) SocketWrapper::readPartMsg();
    if(!m){
        return NULL;
    }
    return decryptMsg(m);
}


Message* SecureSocketWrapper::receiveAnyMsg(){
    LOG(LOG_INFO, "Any message secure socket wrapper");
    SecureMessage* sm = (SecureMessage*) SocketWrapper::receiveAnyMsg();
    return decryptMsg(sm);
}

int SecureSocketWrapper::sendMsg(Message *msg){
    SecureMessage* sm = encryptMsg(msg);
    if(!sm){
        return 1;
    }
    return SocketWrapper::sendMsg(sm);
}

int ClientSecureSocketWrapper::connectServer(Host host){
    int ret;
    *(SocketWrapper::getOtherAddr()) = host.getAddress();

    ret = connect(
        SocketWrapper::getDescriptor(), 
        (struct sockaddr*) SocketWrapper::getOtherAddr(), 
        sizeof(*(SocketWrapper::getOtherAddr()))
    );

    if (ret != 0){
        LOG(LOG_ERR, "Error connecting to %s", 
            sockaddr_in_to_string(host.getAddress()).c_str());
        perror("Error: ");
        return ret;
    }
    
    return ret;
}

ServerSecureSocketWrapper::ServerSecureSocketWrapper(){
    my_addr = make_my_sockaddr_in(0);
    int ret = bind_random_port(SocketWrapper::getDescriptor(), &my_addr);
    if (ret <= 0){
        LOG(LOG_ERR, "Error in binding\n");
        perror("Error: ");        
    }

    ret = listen(SocketWrapper::getDescriptor(), 10);
    if (ret != 0){
        LOG(LOG_ERR, "Error in setting socket to listen mode\n");
        perror("Error: ");        
    }
}

ServerSecureSocketWrapper::ServerSecureSocketWrapper(int port){
    my_addr = make_my_sockaddr_in(port);
    int ret = bind(SocketWrapper::getDescriptor(), (struct sockaddr*) &my_addr, sizeof(my_addr));
    if (ret != 0){
        LOG(LOG_ERR, "Error in binding\n");
        perror("Error: ");        
    }

    ret = listen(SocketWrapper::getDescriptor(), 10);
    if (ret != 0){
        LOG(LOG_ERR, "Error in setting socket to listen mode\n");
        perror("Error: ");        
    }
}

SecureSocketWrapper* ServerSecureSocketWrapper::acceptClient(){
    sockaddr_in* other_addr = SocketWrapper::getOtherAddr();
    socklen_t len = sizeof(*other_addr);

    int new_sd = accept(
        SocketWrapper::getDescriptor(), 
        (struct sockaddr*) other_addr,
        &len
    );

    SecureSocketWrapper *sw = new SecureSocketWrapper(new_sd);
    sw->setOtherAddr(*other_addr);
    return sw;
}