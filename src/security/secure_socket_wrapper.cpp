#include "security/secure_socket_wrapper.h"

SecureSocketWrapper::SecureSocketWrapper() : SocketWrapper()
{
    my_id = (char*) malloc(16);
    get_rand((unsigned char*) my_id, 16);
    memset(my_iv, 0, IV_SIZE);
    memset(other_iv, 0, IV_SIZE);
    buf_idx = 0;
}

SecureSocketWrapper::SecureSocketWrapper(int sd) : SocketWrapper(sd)
{
    my_id = (char*) malloc(16);
    memset(my_iv, 0, IV_SIZE);
    memset(other_iv, 0, IV_SIZE);
    get_rand((unsigned char*) my_id, 16);
    buf_idx = 0;
}

Message *SecureSocketWrapper::decryptMsg(SecureMessage *sm)
{
    msglen_t sm_len = sm->size();
    LOG(LOG_DEBUG, "Received SecureMessage of size %d", sm_len);
    char *buffer = (char *)malloc(sm_len);
    int buf_len = sm->write(buffer);
    msglen_t pt_len = buf_len - 1 - TAG_SIZE;

    LOG(LOG_DEBUG, "Payload");
    DUMP_BUFFER_HEX_DEBUG(buffer+1, pt_len);
    LOG(LOG_DEBUG, "TAG");
    DUMP_BUFFER_HEX_DEBUG(buffer+pt_len+1, TAG_SIZE );

    unsigned char *pt = (unsigned char *)malloc(pt_len);

    int ret = aes_gcm_decrypt((unsigned char *)&buffer[1], pt_len, NULL, 0,
                              (unsigned char *)sym_key, (unsigned char *) other_iv,
                              pt, (unsigned char *)&buffer[buf_len - TAG_SIZE]);
    LOG(LOG_DEBUG, "Decrypted message of size %d with iv", ret);
    DUMP_BUFFER_HEX_DEBUG(other_iv, IV_SIZE);

    if (ret <= 0)
    {
        LOG(LOG_ERR, "Could not decrypt the message");
        return NULL;
    }

    Message *m = readMessage((char *)pt, pt_len);

    other_iv[IV_SIZE-1]++;
    free(buffer);
    free(pt);
    return m;
}

SecureMessage *SecureSocketWrapper::encryptMsg(Message *m)
{
    msglen_t m_len = m->size();
    LOG(LOG_DEBUG, "Encrypting message of size %d", m_len);
    char *buffer = (char *)malloc(m_len);
    int buf_len = m->write(buffer);

    unsigned char *ct = (unsigned char *)malloc(buf_len + 1 + TAG_SIZE);
    unsigned char *tag = (unsigned char *)malloc(TAG_SIZE);
    ct[0] = (MessageType)SECURE_MESSAGE;

    int ret = aes_gcm_encrypt((unsigned char *)buffer, buf_len, NULL, 0,
                              (unsigned char *)sym_key, (unsigned char *) my_iv,
                              &ct[1], tag);
    memcpy(ct + buf_len + 1, tag, TAG_SIZE);

    LOG(LOG_DEBUG, "Message encrypted %d bytes with iv: ", ret);
    DUMP_BUFFER_HEX_DEBUG(my_iv, IV_SIZE);
    LOG(LOG_DEBUG, "and tag: ");
    DUMP_BUFFER_HEX_DEBUG((char*)tag, TAG_SIZE);
    LOG(LOG_DEBUG, "SecureMessage of size %d", buf_len + 1 + TAG_SIZE);

    if (ret <= 0)
    {
        LOG(LOG_ERR, "Could not encrypt the message");
        return NULL;
    }

    SecureMessage *sm = new SecureMessage();
    sm->read((char *)ct, buf_len + 1 + TAG_SIZE);

    my_iv[IV_SIZE-1]++;
    free(ct);
    free(buffer);
    free(tag);
    return sm;
}

Message *SecureSocketWrapper::readPartMsg()
{
    Message *m = SocketWrapper::readPartMsg();
    if (!m)
    {
        return NULL;
    }
    return handleMsg(m);
}

Message *SecureSocketWrapper::receiveAnyMsg()
{
    LOG(LOG_INFO, "Any message secure socket wrapper");
    SecureMessage *sm = (SecureMessage *)SocketWrapper::receiveAnyMsg();
    return handleMsg(sm);
}

Message *SecureSocketWrapper::handleMsg(Message* msg)
{
    int len = msg->size();
    char* buffer = (char*)malloc(len);
    msg->write(buffer);
    switch (buffer[0])
    {
    case SECURE_MESSAGE:
        return decryptMsg((SecureMessage*) msg);    
    case CLIENT_HELLO:
        handleClientHello((ClientHelloMessage*) msg);
        return NULL;
    default:
        break;
    };
}

int SecureSocketWrapper::handleClientHello(ClientHelloMessage* chm)
{
    char* buffer = (char*) malloc(chm->size());
    int len = chm->write(buffer);
    memcpy(other_iv, &buffer[1], sizeof(nonce_t));
    int key_len = len-sizeof(nonce_t)-1;
    EVP_PKEY* client_eph_key = (EVP_PKEY*) malloc(key_len);
    memcpy(client_eph_key, &buffer[len-key_len], key_len);
    LOG(LOG_INFO, "Set other IV to:");
    DUMP_BUFFER_HEX_DEBUG(other_iv, IV_SIZE);
    LOG(LOG_INFO, "Client eph key %d:", key_len);
    DUMP_BUFFER_HEX_DEBUG((char *)client_eph_key, EVP_PKEY_size(client_eph_key));
    return sendServerHello(client_eph_key);
}

int SecureSocketWrapper::sendMsg(Message *msg)
{
    SecureMessage *sm = encryptMsg(msg);
    if (!sm)
    {
        return 1;
    }
    return SocketWrapper::sendMsg(sm);
}

int SecureSocketWrapper::sendClientHello(){
    nonce_t nonce = get_rand();
    EVP_PKEY* eph_key = NULL;
    get_ecdh_key(&eph_key);

    int key_len = EVP_PKEY_size(eph_key);
    LOG(LOG_INFO, "Key len is %d", key_len);
    int len = 1+sizeof(nonce)+key_len;
    char* buffer = (char*) malloc(len);
    memcpy(&buffer[1], &nonce, sizeof(nonce));
    memcpy(&buffer[len-key_len], eph_key, key_len);

    ClientHelloMessage* chm = new ClientHelloMessage;
    chm->read(buffer, len);
    memcpy(my_iv, &nonce, sizeof(nonce));
    LOG(LOG_INFO, "Set my IV to:");
    DUMP_BUFFER_HEX_DEBUG(my_iv, IV_SIZE);
    return SocketWrapper::sendMsg(chm);
}

int SecureSocketWrapper::sendServerHello(EVP_PKEY* client_eph_key){
    nonce_t cl_nonce = (nonce_t) *other_iv;
    nonce_t sv_nonce = get_rand();
    EVP_PKEY* my_eph_key = NULL;
    get_ecdh_key(&my_eph_key);

    unsigned char* dk;

    LOG(LOG_INFO, "My eph key:");
    DUMP_BUFFER_HEX_DEBUG((char *)my_eph_key, EVP_PKEY_size(my_eph_key));
    LOG(LOG_INFO, "Client eph key:");
    DUMP_BUFFER_HEX_DEBUG((char *)client_eph_key, EVP_PKEY_size(client_eph_key));
    int size = dhke(my_eph_key, client_eph_key, dk);

    //Deriving the symmetric key
    hkdf(dk, size, sv_nonce, cl_nonce, NULL, (unsigned char*) sym_key, 16 );
    return 0;
}

int SecureSocketWrapper::handshake(){
    if(!sendClientHello()){
        return 0;
    }
}


int ClientSecureSocketWrapper::connectServer(Host host)
{
    int ret;
    *(SocketWrapper::getOtherAddr()) = host.getAddress();

    ret = connect(
        SocketWrapper::getDescriptor(),
        (struct sockaddr *)SocketWrapper::getOtherAddr(),
        sizeof(*(SocketWrapper::getOtherAddr())));

    if (ret != 0)
    {
        LOG(LOG_ERR, "Error connecting to %s",
            sockaddr_in_to_string(host.getAddress()).c_str());
        perror("Error: ");
        return ret;
    }

    handshake();
    return ret;
}

ServerSecureSocketWrapper::ServerSecureSocketWrapper()
{
    my_addr = make_my_sockaddr_in(0);
    int ret = bind_random_port(SocketWrapper::getDescriptor(), &my_addr);
    if (ret <= 0)
    {
        LOG(LOG_ERR, "Error in binding\n");
        perror("Error: ");
    }

    ret = listen(SocketWrapper::getDescriptor(), 10);
    if (ret != 0)
    {
        LOG(LOG_ERR, "Error in setting socket to listen mode\n");
        perror("Error: ");
    }
}

ServerSecureSocketWrapper::ServerSecureSocketWrapper(int port)
{
    my_addr = make_my_sockaddr_in(port);
    int ret = bind(SocketWrapper::getDescriptor(), (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (ret != 0)
    {
        LOG(LOG_ERR, "Error in binding\n");
        perror("Error: ");
    }

    ret = listen(SocketWrapper::getDescriptor(), 10);
    if (ret != 0)
    {
        LOG(LOG_ERR, "Error in setting socket to listen mode\n");
        perror("Error: ");
    }
}

SecureSocketWrapper *ServerSecureSocketWrapper::acceptClient()
{
    sockaddr_in *other_addr = SocketWrapper::getOtherAddr();
    socklen_t len = sizeof(*other_addr);

    int new_sd = accept(
        SocketWrapper::getDescriptor(),
        (struct sockaddr *)other_addr,
        &len);

    SecureSocketWrapper *sw = new SecureSocketWrapper(new_sd);
    sw->setOtherAddr(*other_addr);
    return sw;
}