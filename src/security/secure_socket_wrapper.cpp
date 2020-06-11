#include "security/secure_socket_wrapper.h"

SecureSocketWrapper::SecureSocketWrapper() : SocketWrapper()
{
    sym_key = "test";
    for (int i = 0; i < IV_SIZE; i++)
    {
        my_iv[i] = 0;
        other_iv[i] = 0;
    }
}

SecureSocketWrapper::SecureSocketWrapper(int sd) : SocketWrapper(sd)
{
    sym_key = "test";
    for (int i = 0; i < IV_SIZE; i++)
    {
        my_iv[i] = 0;
        other_iv[i] = 0;
    }
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
    return decryptMsg((SecureMessage*) m);
}

Message *SecureSocketWrapper::receiveAnyMsg()
{
    LOG(LOG_INFO, "Any message secure socket wrapper");
    SecureMessage *sm = (SecureMessage *)SocketWrapper::receiveAnyMsg();
    return decryptMsg(sm);
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