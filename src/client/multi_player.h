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

#include "security/secure_socket_wrapper.h"
#include "network/host.h"

#define MY_TURN    (0)
#define THEIR_TURN (1)

/**
 * Play against the player at the given socket
 */
int playWithPlayer(int turn, SecureSocketWrapper *sw);

/**
 * Starts a server on the given port waiting for peers to connect.
 */
SecureSocketWrapper* waitForPeer(int port, X509* cert, EVP_PKEY* key, X509_STORE* store);

/**
 * Connects to the server of another peer.
 */
SecureSocketWrapper* connectToPeer(SecureHost peer, X509* cert, EVP_PKEY* key, X509_STORE* store);

#endif // MULTI_PLAYER_H