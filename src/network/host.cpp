/**
 * @file host.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of host.h.
 * 
 * @see host.h
 * 
 * @date 2020-05-20
 */

#include "network/host.h"
#include "network/inet_utils.h"

Host::Host(const char* ip, int port){
    addr = make_sv_sockaddr_in(ip, port);
}
string Host::toString(){      
    return sockaddr_in_to_string(addr);
}