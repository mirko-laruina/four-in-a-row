/**
 * @file host.h
 * @author Riccardo Mancini
 *
 * @brief Definition of the helper class "Host" 
 *
 * @date 2020-05-17
 */

#ifndef HOST_H
#define HOST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "logging.h"

using namespace std;

/**
 * Class that holds a host information
 * 
 * OpenSSL certificates are held in SecureHost class.
 */
class Host{
private:
    struct sockaddr_in addr;

public:
    /** 
     * Constructs new empty instance
     */
    Host() {}
   
    /** 
     * Constructs new instance from given inet address
     * 
     * @param addr the inet address of the remote host
     */
    Host(struct sockaddr_in addr) 
        : addr(addr) {}

    /** 
     * Constructs new instance from IP/port pair
     * 
     * @param ip the IP address the remote host
     * @param port the port the remote host
     */
    Host(char* ip, int port);

    /** Returns the inet address of the host */
    struct sockaddr_in getAddress(){return addr;}

    /** Returns the inet address of the host */
    string toString();

};

#endif // HOST_H