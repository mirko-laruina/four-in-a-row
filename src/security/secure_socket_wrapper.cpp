#include "security/secure_socket_wrapper.h"
#include "security/crypto_utils.h"

SecureSocketWrapper::SecureSocketWrapper(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store)
{
    sw = new SocketWrapper();
    init(cert, my_priv_key, store);
}

SecureSocketWrapper::SecureSocketWrapper(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store, int sd)
{
    sw = new SocketWrapper(sd);
    init(cert, my_priv_key, store);
}

SecureSocketWrapper::SecureSocketWrapper(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store, SocketWrapper *sw)
{
    this->sw = sw;
    init(cert, my_priv_key, store);
}

void SecureSocketWrapper::init(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store){
    send_seq_num = 0;
    recv_seq_num = 0;
    cl_nonce = 0;
    sv_nonce = 0;
    my_eph_key = NULL;
    other_eph_key = NULL;
    this->my_cert = cert;
    this->my_priv_key = my_priv_key;
    this->store = store;
    other_cert = NULL;
    peer_authenticated = false;
    my_id = usernameFromCert(cert);
}

SecureSocketWrapper::~SecureSocketWrapper(){
    delete sw;

    // TODO do not wait this much time to delete them
    if (my_eph_key != NULL){
        EVP_PKEY_free(my_eph_key);
    }
    if (other_eph_key != NULL){
        EVP_PKEY_free(other_eph_key);
    }
    // TODO free certs too?
}

Message *SecureSocketWrapper::decryptMsg(SecureMessage *sm)
{
    if (!peer_authenticated){
        LOG(LOG_WARN, "Unauthenticated peer sent encrypted message");
        return NULL;
    }
    
    msglen_t sm_len = sm->size();
    msglen_t pt_len = sm->size() - TAG_SIZE - 1;
    LOG(LOG_DEBUG, "Received SecureMessage of size %d", sm_len);

    LOG(LOG_DEBUG, "Payload");
    DUMP_BUFFER_HEX_DEBUG(sm->getCt(), pt_len);
    LOG(LOG_DEBUG, "TAG");
    DUMP_BUFFER_HEX_DEBUG(sm->getTag(), TAG_SIZE);

    unsigned char *pt = (unsigned char *)malloc(pt_len);

    updateRecvIV();

    int ret = aes_gcm_decrypt((unsigned char *)sm->getCt(), pt_len, NULL, 0,
                              (unsigned char *)recv_key, (unsigned char *) recv_iv,
                              pt, (unsigned char *)sm->getTag());
    LOG(LOG_DEBUG, "Decrypted message of size %d with iv", ret);
    DUMP_BUFFER_HEX_DEBUG(recv_iv, IV_SIZE);

    if (ret <= 0)
    {
        LOG(LOG_ERR, "Could not decrypt the message");
        return NULL;
    }

    Message *m = readMessage((char *)pt, pt_len);

    free(pt);
    return m;
}

SecureMessage *SecureSocketWrapper::encryptMsg(Message *m)
{
    if (!peer_authenticated)
        return NULL;

    msglen_t m_len = m->size();
    LOG(LOG_DEBUG, "Encrypting message of size %d", m_len);
    char *buffer = (char *)malloc(m_len);
    int buf_len = m->write(buffer);

    unsigned char *ct = (unsigned char *)malloc(buf_len);
    unsigned char *tag = (unsigned char *)malloc(TAG_SIZE);

    updateSendIV();

    int ret = aes_gcm_encrypt((unsigned char *)buffer, buf_len, NULL, 0,
                              (unsigned char *)send_key, (unsigned char *) send_iv,
                              ct, tag);

    LOG(LOG_DEBUG, "Message encrypted %d bytes with iv: ", ret);
    DUMP_BUFFER_HEX_DEBUG(send_iv, IV_SIZE);
    LOG(LOG_DEBUG, "and tag: ");
    DUMP_BUFFER_HEX_DEBUG((char*)tag, TAG_SIZE);
    LOG(LOG_DEBUG, "SecureMessage of size %d", buf_len + 1 + TAG_SIZE);

    if (ret <= 0)
    {
        LOG(LOG_ERR, "Could not encrypt the message");
        return NULL;
    }

    SecureMessage *sm = new SecureMessage(buffer, buf_len + 1 + TAG_SIZE, (char*) tag);

    free(ct);
    free(buffer);
    free(tag);
    return sm;
}

Message *SecureSocketWrapper::readPartMsg()
{
    Message *m = sw->readPartMsg();
    if (!m)
    {
        return NULL;
    }
    return handleMsg(m);
}

Message *SecureSocketWrapper::receiveAnyMsg()
{
    LOG(LOG_INFO, "Any message secure socket wrapper");
    Message *m = sw->receiveAnyMsg();
    
    return handleMsg(m);
}

Message *SecureSocketWrapper::handleMsg(Message* msg)
{
    Message* dm;
    switch(msg->getType()){
        case SECURE_MESSAGE:
            dm = decryptMsg((SecureMessage*) msg);
            if (dm != NULL)
                recv_seq_num++;
            delete msg;
            return dm;
        case CLIENT_HELLO:
            if (cl_nonce == 0 && !peer_authenticated){
                return msg;
            } else{
                // already received
                LOG(LOG_WARN, "Client sent CLIENT_HELLO twice");
                return NULL;
            }
        case SERVER_HELLO:
            if (!peer_authenticated){
                return msg;
            } else{
                // already authenticated!
                LOG(LOG_WARN, "Server sent SERVER_HELLO twice");
                return NULL;
            }
        case CLIENT_VERIFY:
            if (!peer_authenticated){
                return msg;
            } else{
                // already authenticated
                LOG(LOG_WARN, "Client sent CLIENT_VERIFY twice");
                return NULL;
            }
        // TODO certificate req-reply are allowed in cleartext
        default:
            LOG(LOG_WARN, "Peer sent %s in cleartext!", msg->getName().c_str());
            return NULL;
    }
}

int SecureSocketWrapper::handleClientHello(ClientHelloMessage* chm)
{
    cl_nonce = chm->getNonce();
    other_eph_key = chm->getEphKey();
    return sendServerHello();
}

int SecureSocketWrapper::handleServerHello(ServerHelloMessage* shm)
{
    sv_nonce = shm->getNonce();
    other_eph_key = shm->getEphKey();

    unsigned char* dk;
    dk = (unsigned char*) malloc(ECDH_SIZE);

    int size = dhke(my_eph_key, other_eph_key, dk);

    //Deriving the symmetric key
    generateKeys(dk, size, "client");
    free(dk);

    bool check = checkSignature(shm->getDs(), "client");
    if (!check){
        LOG(LOG_ERR, "Digital Signature verification failure!");
        return -1;
    }

    peer_authenticated = true;

    return sendClientVerify();
}

int SecureSocketWrapper::handleClientVerify(ClientVerifyMessage* cvm)
{
    bool check = checkSignature(cvm->getDs(), "server");
    if (!check){
        LOG(LOG_ERR, "Digital Signature verification failure!");
        return -1;
    }

    peer_authenticated = true;

    return 0;
}

int SecureSocketWrapper::sendMsg(Message *msg)
{
    SecureMessage *sm = encryptMsg(msg);
    if (!sm)
    {
        return 1;
    }
    if (sw->sendMsg(sm) == 0){
        send_seq_num++;
        return 0;
    } else{
        return 1;
    }
}

int SecureSocketWrapper::sendClientHello(){
    cl_nonce = get_rand();
    get_ecdh_key(&my_eph_key);

    ClientHelloMessage chm(my_eph_key, cl_nonce, my_id, other_id);
    return sw->sendMsg(&chm);
}

int SecureSocketWrapper::sendServerHello(){
    sv_nonce = get_rand();
    get_ecdh_key(&my_eph_key);

    unsigned char* dk;
    dk = (unsigned char*) malloc(ECDH_SIZE);

    int size = dhke(my_eph_key, other_eph_key, dk);

    //Deriving the symmetric key
    generateKeys(dk, size, "server");
    free(dk);

    char* ds = makeSignature("server");
    ServerHelloMessage shm(my_eph_key, sv_nonce, my_id, other_id, ds); 
    return sw->sendMsg(&shm);
}

int SecureSocketWrapper::sendClientVerify(){
    char* ds = makeSignature("client");
    ClientVerifyMessage cvm(ds); 
    return sw->sendMsg(&cvm);
}

void SecureSocketWrapper::generateKeys(unsigned char* shared_secret, 
                                       int size, const char* role){
    const char* other_role;
    if (strcmp(role, "client") == 0){
        other_role = "server";
    } else if (strcmp(role, "server") == 0){
        other_role = "client";
    } else{
        LOG(LOG_ERR, "Wrong role %s", role);
    }
    char my_key_str[11] = "key_";
    char other_key_str[11] = "key_";
    char my_iv_str[11] = "iv__";
    char other_iv_str[11] = "iv__";
    strcat(my_key_str, role);
    strcat(other_key_str, other_role);
    strcat(my_iv_str, role);
    strcat(other_iv_str, role);

    hkdf(shared_secret, size, sv_nonce, cl_nonce, my_key_str, (unsigned char*) send_key, KEY_SIZE);
    hkdf(shared_secret, size, sv_nonce, cl_nonce, other_key_str, (unsigned char*) recv_key, KEY_SIZE);
    hkdf(shared_secret, size, sv_nonce, cl_nonce, my_iv_str, (unsigned char*) send_iv_static, IV_SIZE);
    hkdf(shared_secret, size, sv_nonce, cl_nonce, other_iv_str, (unsigned char*) recv_iv_static, IV_SIZE);
}

int SecureSocketWrapper::buildMsgToSign(const char* role, char* msg){
    size_t max_size = getLenMsgToSign();
    int i = 0;
    size_t size;
    string A;
    string B;
    EVP_PKEY *A_eph_key;
    EVP_PKEY *B_eph_key;
    if (strcmp(role, "server") == 0){
        A = other_id;
        A_eph_key = other_eph_key;
        B = my_id;
        B_eph_key = my_eph_key;
    } else if (strcmp(role, "client") == 0){
        A = my_id;
        A_eph_key = my_eph_key;
        B = other_id;
        B_eph_key = other_eph_key;
    } else{
        LOG(LOG_ERR, "Unrecognized role %s", role);
        return -1;
    }

    size = min((int)A.size(),MAX_USERNAME_LENGTH);
    strncpy(&msg[i],
         A.c_str(), 
         size);
    i += size;

    size = min((int)B.size(),MAX_USERNAME_LENGTH);
    strncpy(&msg[i],
         B.c_str(), 
         size);
    i += size;

    size = sizeof(nonce_t);
    memcpy(&msg[i], &cl_nonce, size);
    i += size;

    size = sizeof(nonce_t);
    memcpy(&msg[i], &sv_nonce, size);
    i += size;

    size = KEY_BIO_SIZE;
    if (pkey2buf(&A_eph_key, &msg[i], max_size-i) <= 0){
        LOG(LOG_ERR, "Error copying key to buffer");
        return 0;
    }
    i += size;

    size = KEY_BIO_SIZE;
    if (pkey2buf(&B_eph_key, &msg[i], max_size-i) <= 0){
        LOG(LOG_ERR, "Error copying key to buffer");
        return 0;
    }
    i += size;

    return i;
}

size_t SecureSocketWrapper::getLenMsgToSign(){
    return my_id.size() + other_id.size() 
        + sizeof(cl_nonce) + sizeof(sv_nonce)
        + KEY_BIO_SIZE + KEY_BIO_SIZE;
}

char* SecureSocketWrapper::makeSignature(const char* role){
    char* ds = (char*) malloc(DS_SIZE);
    size_t msglen = getLenMsgToSign();
    char* msg = (char*) malloc (msglen);

    if (buildMsgToSign(role, msg) <= 0){
        LOG(LOG_ERR, "Error building message to sign!");
        return NULL;
    }

    if (dsa_sign((unsigned char*) msg, msglen, (unsigned char*) ds, my_priv_key) <= 0){
        free(msg);
        return NULL;
    }

    free(msg);
    return ds;
    
}

bool SecureSocketWrapper::checkSignature(char* ds, const char* role){
    size_t msglen = getLenMsgToSign();
    char* msg = (char*) malloc (msglen);

    if (buildMsgToSign(role, msg) <= 0){
        LOG(LOG_ERR, "Error building message to sign!");
        return NULL;
    }

    bool ret = dsa_verify((unsigned char*) msg, msglen, 
            (unsigned char*) ds, DS_SIZE, 
            X509_get_pubkey(other_cert));

    free(msg);
    return ret;
}

void updateIV(uint64_t seq, char* iv_static, char* iv){
    char* seq_bytes = (char*) &seq;
    for (size_t i=0; i<IV_SIZE; i++){
        if (i<sizeof(seq)){
            iv[i] = iv_static[i] ^ seq_bytes[i];
        } else{
            iv[i] = iv_static[i];
        }
    }
}

void SecureSocketWrapper::updateSendIV(){
    updateIV(send_seq_num, send_iv_static, send_iv);
}

void SecureSocketWrapper::updateRecvIV(){
    updateIV(recv_seq_num, recv_iv_static, recv_iv);
}

int SecureSocketWrapper::handshake(){
    if(sendClientHello() != 0){
        LOG(LOG_ERR, "ClientHello send failed!");
        return 1;
    }

    ServerHelloMessage *shm = dynamic_cast<ServerHelloMessage*>(receiveMsg(SERVER_HELLO));
    if (handleServerHello(shm) != 0){
        LOG(LOG_ERR, "Error handling ServerHello!");
        return 1;
    }
    return sendClientVerify();
}

bool SecureSocketWrapper::setOtherCert(X509* other_cert){
    if (!verify_peer_cert(store, other_cert)){
        LOG(LOG_ERR, "Peer certificate validation failed!");
        return false;
    }
    this->other_cert = other_cert;
    this->other_id = usernameFromCert(other_cert);
    return true;
}

Message* SecureSocketWrapper::receiveMsg(MessageType type){
    return this->receiveMsg(&type, 1);
}

Message* SecureSocketWrapper::receiveMsg(MessageType type[], int n_types){
    Message *m = NULL;
    while (m == NULL){
        try{
            m = receiveAnyMsg();
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

ClientSecureSocketWrapper::ClientSecureSocketWrapper(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store){
    csw = new ClientSocketWrapper();
    sw = csw;
    init(cert, my_priv_key, store);
}

int ClientSecureSocketWrapper::connectServer(SecureHost host)
{
    if (!setOtherCert(host.getCert())){
        LOG(LOG_ERR, "Peer certificate validation failed!");
        return -1;
    }
    return csw->connectServer(host);
}

ServerSecureSocketWrapper::ServerSecureSocketWrapper(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store){
    ssw = new ServerSocketWrapper();
    sw = ssw;
    init(cert, my_priv_key, store);
}

ServerSecureSocketWrapper::ServerSecureSocketWrapper(X509* cert, EVP_PKEY* my_priv_key, X509_STORE* store, int port){
    ssw = new ServerSocketWrapper(port);
    sw = ssw;
    init(cert, my_priv_key, store);
}

SecureSocketWrapper *ServerSecureSocketWrapper::acceptClient()
{
    return new SecureSocketWrapper(my_cert, my_priv_key, store, ssw->acceptClient());
}

SecureSocketWrapper *ServerSecureSocketWrapper::acceptClient(X509* other_cert)
{
    SecureSocketWrapper* sec_sw = new SecureSocketWrapper(my_cert, my_priv_key, store, ssw->acceptClient());
    if (sec_sw->setOtherCert(other_cert))
        return sec_sw;
    else{
        delete sec_sw;
        return NULL;
    }
}