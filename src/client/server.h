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

#include "network/socket_wrapper.h"
#include "network/host.h"

using namespace std;

class Server{
private:
    Host host;
    ClientSocketWrapper* sw;
public:
    Server(Host host) : host(host) {sw = new ClientSocketWrapper();}
    ~Server();
    int registerToServer(string username);
    string getUserList();
    int challengePeer(string username, Host* peerHost);
    int replyPeerChallenge(string username, bool response, Host* peerHost, uint16_t *listen_port);

    SocketWrapper* getSocketWrapper(){return sw;}
};

#endif // SERVER_H