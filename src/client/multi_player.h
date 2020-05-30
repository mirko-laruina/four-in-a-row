/**
 * @file multi_player.h
 * @author Riccardo Mancini
 *
 * @brief Definition of the multi player game main function and connection with
 *          peer functions
 *
 * @date 2020-05-29
 */

#ifndef MULTI_PLAYER_H
#define MULTI_PLAYER_H

#include "network/socket_wrapper.h"
#include "network/host.h"

#define MY_TURN    (0)
#define THEIR_TURN (1)

/**
 * Play against the player at the given socket
 */
int playWithPlayer(int turn, SocketWrapper *sw);

/**
 * Starts a server on the given port waiting for peers to connect.
 */
SocketWrapper* waitForPeer(int port);

/**
 * Connects to the server of another peer.
 */
SocketWrapper* connectToPeer(Host peer);

#endif // MULTI_PLAYER_H