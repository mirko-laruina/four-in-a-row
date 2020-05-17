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

#include "logging.h"
#include "network/inet_utils.h"
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
    Host(char* ip, int port){
        addr = make_sv_sockaddr_in(ip, port);
    }

    /** Returns the inet address of the host */
    struct sockaddr_in getAddress(){return addr;}

    /** Returns the inet address of the host */
    string toString(){      
        return sockaddr_in_to_string(addr);
    }

};

#endif // HOST_H