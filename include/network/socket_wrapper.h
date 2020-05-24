/**
 * @file socket_wrapper.h
 * @author Riccardo Mancini
 *
 * @brief Definition of the helper class "SocketWrapper" and derivatives
 *
 * @date 2020-05-17
 */

#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include "logging.h"
#include "network/messages.h"
#include "network/host.h"

/**
 * Wrapper class around sockaddr_in and socket descriptor
 * 
 * It provides a more simple interface saving a lot of boiler-plate code.
 * There are two subclasses: ClientSocketWrapper and ServerSocketWrapper.
 */ 
class SocketWrapper{
protected:
    /** Other host inet socket */
    struct sockaddr_in other_addr;

    /** Socket file descriptor */
    int socket_fd;

    /** Pre-allocated buffer for incoming messages */
    char buffer[MAX_MSG_SIZE];

    /** Index in the buffer that has been read up to now */
    msglen_t buf_idx;
public:
    /** 
     * Initialize on a new socket
     */
    SocketWrapper();

    /** 
     * Initialize using existing socket
     */
    SocketWrapper(int sd) : socket_fd(sd) {}

    ~SocketWrapper(){closeSocket();}

    /** 
     * Returns current socket file descriptor
     */
    int getDescriptor(){return socket_fd;};
    
    /** 
     * Read any new data from the socket but does not wait for the 
     * whole message to be ready.
     * 
     * This API is blocking iff socket was not ready.
     * 
     * @param size the size of the temporary buffer
     * @returns the received message or null if an error occurred
     */
    Message* readPartMsg();

    /** 
     * Receive any new message from the socket.
     * 
     * This API is blocking.
     * 
     * @returns the received message or null if an error occurred
     */
    Message* receiveAnyMsg();

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
    Message* receiveMsg(MessageType type);

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
    Message* receiveMsg(MessageType type[], int n_types);

    /**
     * Sends the given message to the peer host through the socket.
     * 
     * @param msg the message to be sent
     * @returns 0 in case of success, something else otherwise
     */
    int sendMsg(Message *msg);

    /**
     * Closes the socket.
     */
    void closeSocket();

    /**
     * Sets the address of the other host.
     * 
     * This is used when initializing a new SocketWrapper for a newly 
     * accepter connection.
     */
    void setOtherAddr(struct sockaddr_in addr){other_addr = addr;}

    /**
     * Returns connected host.
     */
    Host getConnectedHost(){return Host(other_addr);}
};

/**
 * SocketWrapper for a TCP client
 * 
 * It provides a new function to connect to server.
 */
class ClientSocketWrapper : public SocketWrapper{
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
class ServerSocketWrapper : public SocketWrapper{
private:
    /** Local inet socket */
    struct sockaddr_in my_addr;
public:
    /** 
     * Initialize a new socket on a random port.
     * 
     * @param port the port you want to bind on 
     */
    ServerSocketWrapper();

    /** 
     * Initialize a new socket at the requested port.
     * 
     * @param port the port you want to bind on 
     */
    ServerSocketWrapper(int port);

    /**
     * Accepts any incoming connection and returns the related SocketWrapper.
     */
    SocketWrapper* acceptClient();

    /**
     * Returns port the server is listening new connections on.
     */
    int getPort(){return ntohs(my_addr.sin_port);}
};

#endif // SOCKET_WRAPPER_H