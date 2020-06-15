/**
 * @file secure_host.h
 * @author Riccardo Mancini
 *
 * @brief Definition of the helper class "SecureHost" 
 *
 * @date 2020-06-11
 */

#ifndef SECURE_HOST_H
#define SECURE_HOST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "logging.h"
#include "network/host.h"
#include "security/crypto.h"

using namespace std;

/**
 * Class that holds a host information with certificate
 */
class SecureHost : public Host{
private:
    X509* cert;
public:
    /** 
     * Constructs new empty instance
     */
    SecureHost() {}
   
    /** 
     * Constructs new instance from given inet address and X509 certificate
     * 
     * @param addr the inet address of the remote host
     * @param cert X509 certificate
     */
    SecureHost(struct sockaddr_in addr, X509* cert) : Host(addr), cert(cert) {}

    /** 
     * Constructs new instance from IP/port pair
     * 
     * @param ip the IP address the remote host
     * @param port the port the remote host
     */
    SecureHost(const char* ip, int port, X509* cert) : Host(ip, port), cert(cert) {}

    /** Returns the X509 certificate */
    X509* getCert(){return cert;}
};

#endif // SECURE_HOST_H