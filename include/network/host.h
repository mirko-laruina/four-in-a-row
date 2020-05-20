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
#include "logging.h"
#include "network/messages.h"

/**
 * Class that holds a host information
 * 
 * At the moment, it only holds its inet addr but in the future its 
 * public key and other OpenSSL stuff will be put here.
 */
class Host{
private:
    struct sockaddr_in addr;

public:
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
     * @param addr the inet address of the remote host
     */
    Host(char* ip, int port);

    /** Returns the inet address of the host */
    struct sockaddr_in getAddress(){return addr;}

    /** Returns the inet address of the host */
    string toString();

};

#endif // HOST_H