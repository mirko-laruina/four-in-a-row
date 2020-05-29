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

ConnectionMode serverLobby(Host host);

#endif // SERVER_LOBBY_H
