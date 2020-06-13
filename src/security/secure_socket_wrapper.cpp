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
    int ret;

    msglen_t pt_len = sm->getCtSize();
    LOG(LOG_DEBUG, "Received SecureMessage of size %d", pt_len);

    LOG(LOG_DEBUG, "Payload");
    DUMP_BUFFER_HEX_DEBUG(sm->getCt(), pt_len);
    LOG(LOG_DEBUG, "TAG");
    DUMP_BUFFER_HEX_DEBUG(sm->getTag(), TAG_SIZE);

    char *buffer_pt = (char*) malloc(pt_len);
    char buffer_aad[AAD_SIZE];

    makeAAD(SECURE_MESSAGE, pt_len+TAG_SIZE+AAD_SIZE, buffer_aad);
    DUMP_BUFFER_HEX_DEBUG(buffer_aad, AAD_SIZE);

    updateRecvIV();

    ret = aes_gcm_decrypt(sm->getCt(), pt_len, buffer_aad, AAD_SIZE,
                              recv_key, recv_iv,
                              buffer_pt, sm->getTag());

    if (ret <= 0)
    {
        LOG(LOG_ERR, "Could not decrypt the message");
        return NULL;
    }

    LOG(LOG_DEBUG, "Decrypted message (%d):", ret);
    DUMP_BUFFER_HEX_DEBUG(buffer_pt, ret);

    Message *m = readMessage(buffer_pt, pt_len);

    free(buffer_pt);

    if (m != NULL){
        LOG(LOG_DEBUG, "Decrypted message of type %s", m->getName().c_str());
    } else{
        LOG(LOG_DEBUG, "Malformed message");
    }
    return m;
}

SecureMessage *SecureSocketWrapper::encryptMsg(Message *m)
{
    if (!peer_authenticated)
        return NULL;
    int ret;

    char buffer_pt[MAX_MSG_SIZE];
    int buf_len = m->write(buffer_pt);

    char* buffer_ct = (char*) malloc(MAX_MSG_SIZE);
    char* buffer_tag = (char*) malloc(TAG_SIZE);
    char  buffer_aad[AAD_SIZE];

    LOG(LOG_DEBUG, "Encrypting message of size %d", buf_len);

    updateSendIV();
    makeAAD(SECURE_MESSAGE, buf_len+TAG_SIZE+AAD_SIZE, buffer_aad);
    DUMP_BUFFER_HEX_DEBUG(buffer_aad, AAD_SIZE);

    ret = aes_gcm_encrypt(buffer_pt, buf_len, buffer_aad, AAD_SIZE,
                              send_key, send_iv,
                              buffer_ct, buffer_tag);

    LOG(LOG_DEBUG, "Message encrypted %d bytes with iv: ", ret);
    DUMP_BUFFER_HEX_DEBUG(send_iv, IV_SIZE);
    LOG(LOG_DEBUG, "and tag: ");
    DUMP_BUFFER_HEX_DEBUG(buffer_tag, TAG_SIZE);
    LOG(LOG_DEBUG, "SecureMessage of size %d", buf_len + 1 + TAG_SIZE);

    if (ret <= 0)
    {
        LOG(LOG_ERR, "Could not encrypt the message");
        return NULL;
    }

    SecureMessage *sm = new SecureMessage(buffer_ct, ret, buffer_tag);

    return sm;
}

void SecureSocketWrapper::makeAAD(MessageType msg_type, msglen_t len, char* aad){
    *((msglen_t*)aad) = MSGLEN_HTON(len);
    aad[2] = msg_type;
}

Message *SecureSocketWrapper::readPartMsg()
{
    return sw->readPartMsg();
}

Message *SecureSocketWrapper::receiveAnyMsg()
{
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

    //Deriving the symmetric key
    generateKeys("client");

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

    LOG(LOG_INFO, "Digital Signature verification succeded!");

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
    int ret = sw->sendMsg(sm);
    delete sm;
    if (ret == 0){
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

    //Deriving the symmetric key
    generateKeys("server");

    char* ds = makeSignature("server");
    ServerHelloMessage shm(my_eph_key, sv_nonce, my_id, other_id, ds); 
    return sw->sendMsg(&shm);
}

int SecureSocketWrapper::sendClientVerify(){
    char* ds = makeSignature("client");
    ClientVerifyMessage cvm(ds); 
    return sw->sendMsg(&cvm);
}

void SecureSocketWrapper::generateKeys(const char* role){
    char *shared_secret = NULL;

    int size = dhke(my_eph_key, other_eph_key, &shared_secret);

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
    strcat(other_iv_str, other_role);

    hkdf(shared_secret, size, sv_nonce, cl_nonce, my_key_str, send_key, KEY_SIZE);
    hkdf(shared_secret, size, sv_nonce, cl_nonce, other_key_str, recv_key, KEY_SIZE);
    hkdf(shared_secret, size, sv_nonce, cl_nonce, my_iv_str, send_iv_static, IV_SIZE);
    hkdf(shared_secret, size, sv_nonce, cl_nonce, other_iv_str, recv_iv_static, IV_SIZE);

    LOG(LOG_DEBUG, "HKDF parameters BEGIN --------");
    LOG(LOG_DEBUG, "Shared secret:");
    DUMP_BUFFER_HEX_DEBUG(shared_secret, size);
    LOG(LOG_DEBUG, "sv_nonce=%d", sv_nonce);
    LOG(LOG_DEBUG, "cl_nonce=%d", cl_nonce);
    LOG(LOG_DEBUG, "HKDF parameters END --------");
    LOG(LOG_DEBUG, "Generated keys BEGIN --------");
    LOG(LOG_DEBUG, "Send key (%s):", my_key_str);
    DUMP_BUFFER_HEX_DEBUG(send_key, KEY_SIZE);
    LOG(LOG_DEBUG, "Send IV (%s):", my_iv_str);
    DUMP_BUFFER_HEX_DEBUG(send_iv_static, IV_SIZE);
    LOG(LOG_DEBUG, "Recv key (%s):", other_key_str);
    DUMP_BUFFER_HEX_DEBUG(recv_key, KEY_SIZE);
    LOG(LOG_DEBUG, "Recv IV (%s):", other_iv_str);
    DUMP_BUFFER_HEX_DEBUG(recv_iv_static, IV_SIZE);
    LOG(LOG_DEBUG, "Generated keys END --------");

    free(shared_secret);
}

int SecureSocketWrapper::buildMsgToSign(const char* role, char* msg){
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

    size = pkey2buf(&A_eph_key, &msg[i], MAX_MSG_TO_SIGN_SIZE-i);
    if (size <= 0){
        LOG(LOG_ERR, "Error copying key to buffer");
        return 0;
    }
    i += size;

    size = pkey2buf(&B_eph_key, &msg[i], MAX_MSG_TO_SIGN_SIZE-i);
    if (size <= 0){
        LOG(LOG_ERR, "Error copying key to buffer");
        return 0;
    }
    i += size;

    return i;
}

char* SecureSocketWrapper::makeSignature(const char* role){
    char* ds = (char*) malloc(DS_SIZE);

    size_t msglen = buildMsgToSign(role, msg_to_sign_buf);
    if (msglen <= 0){
        LOG(LOG_ERR, "Error building message to sign!");
        return NULL;
    }

    if (dsa_sign(msg_to_sign_buf, msglen, ds, my_priv_key) <= 0){
        return NULL;
    }

    return ds;    
}

bool SecureSocketWrapper::checkSignature(char* ds, const char* role){
    size_t msglen = buildMsgToSign(role, msg_to_sign_buf);

    if (msglen <= 0){
        LOG(LOG_ERR, "Error building message to sign!");
        return false;
    }

    bool ret = dsa_verify(msg_to_sign_buf, msglen, ds, DS_SIZE, 
                          X509_get_pubkey(other_cert));

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

int SecureSocketWrapper::handshakeClient(){
    if(sendClientHello() != 0){
        LOG(LOG_ERR, "ClientHello send failed!");
        return 1;
    }
    LOG(LOG_INFO, "Client Hello sent");
    ServerHelloMessage *shm = dynamic_cast<ServerHelloMessage*>(receiveMsg(SERVER_HELLO));
    if (handleServerHello(shm) != 0){
        LOG(LOG_ERR, "Error handling ServerHello!");
        return 1;
    } else{
        LOG(LOG_INFO, "Server Hello handled");
        return 0;
    }
}

int SecureSocketWrapper::handshakeServer(){
    ClientHelloMessage *chm = dynamic_cast<ClientHelloMessage*>(receiveMsg(CLIENT_HELLO));

    if (handleClientHello(chm) != 0){
        LOG(LOG_ERR, "Error handling ClientHello!");
        return 1;
    }

    ClientVerifyMessage *cvm = dynamic_cast<ClientVerifyMessage*>(receiveMsg(CLIENT_VERIFY));

    if (handleClientVerify(cvm) != 0){
        LOG(LOG_ERR, "Error handling ClientVerify!");
        return 1;
    }
    
    return 0;
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