/**
 * @file secure_socket_wrapper.h
 * @author Mirko Laruina
 * 
 * @brief Header file for SecureSocketWrapper
 * 
 * @date 2020-06-09 
 */

#ifndef SECURE_SOCKET_WRAPPER_H
#define SECURE_SOCKET_WRAPPER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include "logging.h"
#include "network/messages.h"
#include "network/socket_wrapper.h"
#include "security/crypto.h"
#include "security/crypto_utils.h"
#include "security/secure_host.h"
#include "utils/dump_buffer.h"

#define MAX_MSG_TO_SIGN_SIZE (2*MAX_USERNAME_LENGTH + 2 * sizeof(nonce_t) + 2 * KEY_BIO_MAX_SIZE )
#define MAX_SEC_MSG_SIZE (MAX_MSG_SIZE - TAG_SIZE - sizeof(msglen_t) - 1)

#define AAD_SIZE (sizeof(msglen_t) + 1)

class SecureSocketWrapper
{
protected:
    SocketWrapper *sw;

    char send_key[KEY_SIZE];
    char recv_key[KEY_SIZE];
    char send_iv_static[IV_SIZE];
    char recv_iv_static[IV_SIZE];
    char send_iv[IV_SIZE];
    char recv_iv[IV_SIZE];
    uint64_t send_seq_num;
    uint64_t recv_seq_num;
    string my_id;
    string other_id;
    nonce_t sv_nonce;
    nonce_t cl_nonce;
    EVP_PKEY *my_eph_key;
    EVP_PKEY *other_eph_key;
    X509 *my_cert;
    X509 *other_cert;
    X509_STORE *store;
    EVP_PKEY *my_priv_key;

    bool peer_authenticated;

    char msg_to_sign_buf[MAX_MSG_TO_SIGN_SIZE];

    /** 
     * Empty constructor to use in child classes.
     */
    SecureSocketWrapper(){};

    void generateKeys(const char *role);
    void updateSendIV();
    void updateRecvIV();

    /** Internal initialization */
    void init(X509 *cert, EVP_PKEY *my_priv_key, X509_STORE *store);

    /**
     * @brief Decrypts a Secure Message into a Message
     * 
     * @param sm             Secure message ptr
     * @return Message*      Read message
     */
    Message *decryptMsg(SecureMessage *sm);

    /**
     * @brief Encrypts a Message into a SecureMessage
     * 
     * @param m                   Message to encrypt
     * @return SecureMessage*     Encrypted SecureMessage
     */
    SecureMessage *encryptMsg(Message *m);

    /**
     * Make the signature for the handshake protocol
     */
    char *makeSignature(const char *role);

    /**
     * Checks the signature for the handshake protocol
     */
    bool checkSignature(char *ds, const char *role);

    /**
     * Builds the message to be signed.
     * 
     * @param role the role of this peer
     * @param msg the buffer to write the message to
     * @returns number of written bytes
     */
    int buildMsgToSign(const char *role, char *msg);

    /**
     * Builds the aad of a message.
     * 
     * I.e. this function builds the message header as SocketWrapper would.
     * 
     * @param msg_type the type of the message
     * @param len the length of the message
     * @param aad the aad buffer to write to (it must be AAD_SIZE long)
     * @returns number of written bytes
     * 
     * @see AAD_SIZE
     */
    void makeAAD(MessageType msg_type, msglen_t len, char* aad);

public:
    /** 
     * Initialize on a new socket
     */
    SecureSocketWrapper(X509 *cert, EVP_PKEY *my_priv_key, X509_STORE *store);

    /** 
     * Initialize using existing socket
     */
    SecureSocketWrapper(X509 *cert, EVP_PKEY *my_priv_key, X509_STORE *store, int sd);

    /** 
     * Constructor to generate connection socket wrappers
     */
    SecureSocketWrapper(X509 *cert, EVP_PKEY *my_priv_key, X509_STORE *store, SocketWrapper *sw);

    /** 
     * Destructor
     */
    ~SecureSocketWrapper();

    /** 
     * Read any new data from the socket but does not wait for the 
     * whole message to be ready. This does not decrypt the message!
     * 
     * This API is blocking iff socket was not ready.
     * 
     * @param size the size of the temporary buffer
     * @returns the received message or null if an error occurred
     */
    Message *readPartMsg();

    /** 
     * Receive any new message from the socket.
     * 
     * This API is blocking.
     * 
     * @returns the received message or null if an error occurred
     */
    Message *receiveAnyMsg();

    /** 
     * Receive a new message of the given type from the socket.
     * 
     * When a message of the wrong type is received it is simply ignored.
     * 
     * This API is blocking.
     * 
     * @param type the type to keep
     * @returns the received message or null if an error occurred
     */
    Message *receiveMsg(MessageType type);

    /** 
     * Receive a new message of any of the given types from the socket.
     * 
     * When a message of the wrong type is received it is simply ignored.
     * 
     * This API is blocking.
     * 
     * @param type the types to keep (array)
     * @param n_types the number of types to keep (array length)
     * @returns the received message or null if an error occurred
     */
    Message *receiveMsg(MessageType type[], int n_types);

    Message *handleMsg(Message *msg);

    int sendCertRequest();
    int handleCertResponse(CertificateMessage* cm);

    int handleClientHello(ClientHelloMessage *chm);
    int handleServerHello(ServerHelloMessage *shm);
    int handleClientVerify(ClientVerifyMessage *cvm);

    int sendClientHello();
    int sendServerHello();
    int sendClientVerify();

    int sendPlain(Message *msg);
    /**
     * Sends the given message to the peer host through the socket.
     * 
     * @param msg the message to be sent
     * @returns 0 in case of success, something else otherwise
     */
    int sendMsg(Message *msg);

    /**
     * @brief Estiblishes a secure connection over the already specified socket. To be run server-side.
     * 
     * @return int  0 in case of success, something else otherwise
     */
    int handshakeServer();

    /**
     * @brief Estiblishes a secure connection over the already specified socket. To be run client-side.
     * 
     * @return int  0 in case of success, something else otherwise
     */
    int handshakeClient();

    /**
     * Sets the peer certificate.
     */
    bool setOtherCert(X509 *other_cert);

    /** 
     * Returns current socket file descriptor
     */
    int getDescriptor() { return sw->getDescriptor(); };

    /**
     * Closes the socket.
     */
    void closeSocket() { sw->closeSocket(); }

    /**
     * Sets the address of the other host.
     * 
     * This is used when initializing a new SocketWrapper for a newly 
     * accepter connection.
     */
    void setOtherAddr(struct sockaddr_in addr) { sw->setOtherAddr(addr); }

    sockaddr_in *getOtherAddr() { return sw->getOtherAddr(); }

    /**
     * Returns connected host.
     */
    SecureHost getConnectedHost() { return SecureHost(*getOtherAddr(), other_cert); }

    /**
     * Returns the certificate of this host
     */
    X509* getCert(){ return my_cert;}
};

/**
 * SocketWrapper for a TCP client
 * 
 * It provides a new function to connect to server.
 */
class ClientSecureSocketWrapper : public SecureSocketWrapper
{
private:
    ClientSocketWrapper *csw;

public:
    /** 
     * Initialize a new socket on a random port.
     * 
     * @param port the port you want to bind on 
     */
    ClientSecureSocketWrapper(X509 *cert, EVP_PKEY *my_priv_key, X509_STORE *store);

    /**
     * Connects to a remote server.
     * 
     * @returns 0 in case of success, something else otherwise
     */
    int connectServer(SecureHost host);

};

/**
 * SocketWrapper for a TCP server
 * 
 * It provides a new function to accept clients.
 * Constructor also set listen mode.
 */
class ServerSecureSocketWrapper : public SecureSocketWrapper
{
private:
    ServerSocketWrapper *ssw;

public:
    /** 
     * Initialize a new socket on a random port.
     * 
     * @param port the port you want to bind on 
     */
    ServerSecureSocketWrapper(X509 *cert, EVP_PKEY *my_priv_key, X509_STORE *store);

   /** 
     * Binds the socket to the requested port.
     * 
     * @param port the port you want to bind on 
     * @returns 0 in case of success
     * @returns 1 otherwise
     */
    int bindPort(int port) { return ssw->bindPort(port); }

    /** 
     * Binds the socket to a random port.
     * 
     * @returns 0 in case of success
     * @returns 1 otherwise 
     */
    int bindPort() { return ssw->bindPort(); }

    /**
     * Accepts any incoming connection and returns the related SocketWrapper.
     */
    SecureSocketWrapper *acceptClient();

    /**
     * Accepts any incoming connection and returns the related SocketWrapper.
     * 
     * The certificate is set as the expected certificate of the peer.
     */
    SecureSocketWrapper *acceptClient(X509 *other_cert);

    /**
     * Returns port the server is listening new connections on.
     */
    int getPort() { return ssw->getPort(); }
};

#endif
