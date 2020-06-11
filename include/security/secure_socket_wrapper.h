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
#include "network/host.h"
#include "network/socket_wrapper.h"
#include "security/crypto.h"
#include "utils/dump_buffer.h"

class SecureSocketWrapper : public SocketWrapper
{
protected:
     char sym_key[16];
     char my_iv[IV_SIZE];
     char other_iv[IV_SIZE];
     char* my_id;
     char* other_id;

     int sendClientHello();
     int sendServerHello(EVP_PKEY* client_eph_key);

public:
     /** 
         * Initialize on a new socket
         */
     SecureSocketWrapper();

     /** 
         * Initialize using existing socket
         */
     SecureSocketWrapper(int sd);

     /** 
         * Read any new data from the socket but does not wait for the 
         * whole message to be ready.
         * 
         * This API is blocking iff socket was not ready.
         * 
         * @param size the size of the temporary buffer
         * @returns the received message or null if an error occurred
         */
     Message *readPartMsg();

     /**
      * @brief Decrypts a Secure Message into a Message
      * 
      * @param sm             Secure message ptr
      * @return Message*      Read message
      */
     Message* decryptMsg(SecureMessage* sm);

     /**
      * @brief Encrypts a Message into a SecureMessage
      * 
      * @param m                   Message to encrypt
      * @return SecureMessage*     Encrypted SecureMessage
      */
     SecureMessage* encryptMsg(Message* m);

     /** 
         * Receive any new message from the socket.
         * 
         * This API is blocking.
         * 
         * @returns the received message or null if an error occurred
         */
     Message* receiveAnyMsg();


     Message* handleMsg(Message* msg);
     int handleClientHello(ClientHelloMessage* chm);
     /**
         * Sends the given message to the peer host through the socket.
         * 
         * @param msg the message to be sent
         * @returns 0 in case of success, something else otherwise
         */
     int sendMsg(Message *msg);

    /**
     * @brief Estiblishes a secure connection over the already specified socket
     * 
     * @return int  0 in case of success, something else otherwise
     */
     int handshake();

};

/**
 * SocketWrapper for a TCP client
 * 
 * It provides a new function to connect to server.
 */
class ClientSecureSocketWrapper : public SecureSocketWrapper
{
public:
     /**
     * Connects to a remote server.
     * 
     * @returns 0 in case of success, something else otherwise
     */
     int connectServer(Host host);
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
     /** Local inet socket */
     struct sockaddr_in my_addr;

public:
     /** 
     * Initialize a new socket on a random port.
     * 
     * @param port the port you want to bind on 
     */
     ServerSecureSocketWrapper();

     /** 
     * Initialize a new socket at the requested port.
     * 
     * @param port the port you want to bind on 
     */
     ServerSecureSocketWrapper(int port);

     /**
     * Accepts any incoming connection and returns the related SocketWrapper.
     */
     SecureSocketWrapper *acceptClient();

     /**
     * Returns port the server is listening new connections on.
     */
     int getPort() { return ntohs(my_addr.sin_port); }
};

#endif
