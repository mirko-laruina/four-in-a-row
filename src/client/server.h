/**
 * @file server.h
 * @author Riccardo Mancini
 *
 * @brief Definition of the utility class used to communicate with the server
 *
 * @date 2020-05-29
 */

#ifndef SERVER_H
#define SERVER_H

#include "security/secure_socket_wrapper.h"
#include "security/secure_host.h"
#include "security/crypto_utils.h"

using namespace std;

/**
 * Utility class for interacting with the server
 */
class Server{
private:
    SecureHost host;
    ClientSecureSocketWrapper* sw;
    bool connected;
public:
    /**
     * Constructor
     */
    Server(SecureHost host, X509* cert, EVP_PKEY* key, X509_STORE* store) : host(host) {sw = new ClientSecureSocketWrapper(cert, key, store);}

    /** 
     * Destructor
     */
    ~Server();

    int getServerCert();
    /**
     * Registers the user in the server.
     * 
     * This function also connects the socket if not already done.
     * 
     * Username is inferred from the certificate
     * 
     * @return 0 in case of success, 1 in case of error
     */
    int registerToServer();

    /**
     * Returns the list of available users in the server as a comma separated 
     * list.
     * 
     * @return the list of users.
     */
    string getUserList();

    /**
     * Challenges the given peer and wait for a reply.
     * 
     * TODO: add timeout and possibility to interrupt ?
     * 
     * @param username the username of the peer to challenge
     * @param peerHost a pointer to the structure that will be filled with the 
     *                 peer connection parameters in case the challenge is 
     *                 accepted
     * @returns 0  in case of accepted challenge
     * @returns -1 in case of refused challenge
     * @returns 1  in case of connection failures
     */
    int challengePeer(string username, SecureHost* peerHost);

    /**
     * Replies to the challenge of another user.
     * 
     * @param username the username of the user that sent the challenge
     * @param response the reply (true => accept)
     * @param peerHost a pointer to the structure that will be filled with the 
     *                 peer connection parameters in case the challenge is 
     *                 accepted
     * @returns 0  in case of accepted challenge
     * @returns -1 in case of refused challenge
     * @returns 1  in case of connection failures
     */
    int replyPeerChallenge(string username, bool response, SecureHost* peerHost, uint16_t *listen_port);

    /**
     * Signals the server that the user finished his game.
     * 
     * @returns 0 in case of success
     * @returns 1 in case message could not be delivered
     */
    int signalGameEnd();

    /**
     * Disconnects from the server
     */
    void disconnect();

    /**
     * Returns the internal SocketWrapper.
     */
    SecureSocketWrapper* getSocketWrapper(){return sw;}

    /**
     * Returns the internal Host.
     */
    SecureHost getHost(){return host;}

    /**
     * Returns the player username from his certificate
     */
    string getPlayerUsername(){ return usernameFromCert(sw->getCert());}

    /**
     * Returns whether server is connected
     */
    bool isConnected(){ return connected; }
};

#endif // SERVER_H