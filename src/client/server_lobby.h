/**
 * @file server_lobby.h
 * @author Riccardo Mancini
 *
 * @brief Definition of the function that handles user and network input while 
 *          the user is in the server lobby waiting for a game to start
 *
 * @date 2020-05-29
 */

#ifndef SERVER_LOBBY_H
#define SERVER_LOBBY_H

#include "connection_mode.h"
#include "network/host.h"

/**
 * Handles interaction among the user, the client and the remote server.
 * 
 * While in the lobby, users can receive challenges from other users through
 * the server, request the list of users in the server and challenge other 
 * users.
 * 
 * Once started, this function spawns a new connection to the given host and 
 * registers to it.
 */
ConnectionMode serverLobby(Host host);

#endif // SERVER_LOBBY_H
