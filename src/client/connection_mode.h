/**
 * @file connection_mode.h
 * @author Riccardo Mancini
 *
 * @brief Header file for utility structure ConnectionMode
 *
 * @date 2020-05-29
 */
#ifndef CONNECTION_MODE_H
#define CONNECTION_MODE_H

#include "network/host.h"

enum ConnectionType {CONNECT_TO_SERVER, CONNECT_TO_PEER, WAIT_FOR_PEER, SINGLE_PLAYER, EXIT};

struct ConnectionMode {
    enum ConnectionType connection_type;
    union{
        Host host;
        int listen_port;
        int exit_code;
    };
    ConnectionMode(enum ConnectionType connection_type, 
                        int listen_port) 
            : connection_type(connection_type), listen_port(listen_port) {}
    ConnectionMode(enum ConnectionType connection_type, 
                        char* ip, int port) 
            : connection_type(connection_type), host(Host(ip, port)) {}
    ConnectionMode(enum ConnectionType connection_type, Host host) 
            : connection_type(connection_type), host(host) {}

};

#endif //CONNECTION_MODE_H
