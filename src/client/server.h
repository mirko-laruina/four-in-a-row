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
#include "network/host.h"

using namespace std;

/**
 * Utility class for interacting with the server
 */
class Server{
private:
    Host host;
    ClientSecureSocketWrapper* sw;
public:
    /**
     * Constructor
     */
    Server(Host host) : host(host) {sw = new ClientSecureSocketWrapper();}

    /** 
     * Destructor
     */
    ~Server();

    /**
     * Registers the user in the server.
     * 
     * This function also connects the socket if not already done.
     * 
     * @param username the username of the user connecting to the server
     * @return 0 in case of success, 1 in case of error
     */
    int registerToServer(string username);

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
    int challengePeer(string username, Host* peerHost);

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
    int replyPeerChallenge(string username, bool response, Host* peerHost, uint16_t *listen_port);

    /**
     * Returns the internal SocketWrapper.
     */
    SecureSocketWrapper* getSocketWrapper(){return sw;}
};

#endif // SERVER_H