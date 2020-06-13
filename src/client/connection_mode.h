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

#include "security/secure_host.h"

/** 
 * Type of gmae connection requested by the user:
 * CONNECT_TO_SERVER: the user connects to the given server (Host) that manages 
 *      users and forwards challenges between users.
 * CONNECT_TO_PEER: the user directly connects to the given peer (Host) for a
 *      game.
 * WAIT_FOR_PEER: the user waits for requests from other peers on the given 
 *      port and accepts any incoming game.
 * SINGLE_PLAYER: the player plays against an AI (tbf it's random).
 * EXIT: (used internally) exit the game with the given return code.
 */
enum ConnectionType {CONNECT_TO_SERVER, CONNECT_TO_PEER, WAIT_FOR_PEER, SINGLE_PLAYER, EXIT, CONTINUE};

enum ExitCode {OK, CONNECTION_ERROR, GENERIC_ERROR, FATAL_ERROR};

/**
 * Structure holding information about the connection requested by the user.
 * 
 * @see ConnectionType
 */
struct ConnectionMode {
    enum ConnectionType connection_type;
    SecureHost host;
    union{
        uint16_t listen_port;
        enum ExitCode exit_code;
    };
    ConnectionMode(enum ConnectionType connection_type, 
                        char* ip, int port, X509* cert, uint16_t listen_port) 
            : connection_type(connection_type), host(SecureHost(ip, port,cert)), listen_port(listen_port) {}
    ConnectionMode(enum ConnectionType connection_type, SecureHost host, uint16_t listen_port) 
            : connection_type(connection_type), host(host), listen_port(listen_port) {}

    ConnectionMode(enum ConnectionType connection_type, enum ExitCode exit_code) 
            : connection_type(connection_type), exit_code(exit_code) {}

    ConnectionMode(enum ConnectionType connection_type) 
            : connection_type(connection_type) {}

};

#endif //CONNECTION_MODE_H
