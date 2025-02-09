/**
 * @file server.h
 * @author Riccardo Mancini
 *
 * @brief Implementation of the utility class used to communicate with the server
 *
 * @date 2020-05-29
 */

#include "server.h"
#include "network/messages.h"
#include "network/inet_utils.h"
#include <iostream>

Server::~Server()
{
    if (sw != NULL)
    {
        sw->closeSocket();
        delete sw;
    }
}

int Server::getServerCert()
{
    sw->sendCertRequest();
    CertificateMessage *crm = dynamic_cast<CertificateMessage *>(sw->receiveMsg(CERTIFICATE));
    if (crm != NULL && sw->setOtherCert(crm->getCert()))
    {
        return 0;
    }
    return 1;
}

int Server::registerToServer()
{
    if (sw->connectServer(host) != 0)
    {
        connected = false;
        return 1;
    }

    if (host.getCert() == NULL)
    {
        if (getServerCert() != 0)
        {
            connected = false;
            return 1;
        }
    }

    if (sw->handshakeClient() != 0)
    {
        connected = false;
        return 1;
    }

    RegisterMessage msg(getPlayerUsername());

    int ret = sw->sendMsg(&msg);
    connected = ret == 0;
    return ret;
}

string Server::getUserList()
{
    UsersListRequestMessage req_msg;

    if (sw->sendMsg(&req_msg) != 0)
    {
        connected = false;
        return "";
    }

    UsersListMessage *res_msg;

    try
    {
        res_msg = dynamic_cast<UsersListMessage *>(sw->receiveMsg(USERS_LIST));
    }
    catch (const char *error_msg)
    {
        cerr << "Could not connect to server " << host.toString();
        cerr << " : " << error_msg << endl;
        connected = false;
        return "";
    }

    if (res_msg == NULL)
        return "";

    string usernames = res_msg->getUsernames();
    delete res_msg;

    return usernames;
}

int Server::challengePeer(string username, SecureHost *peerHost)
{
    ChallengeMessage req_msg(username);

    if (sw->sendMsg(&req_msg) != 0)
    {
        connected = false;
        return 1;
    }
    Message *res_msg;

    try
    {
        MessageType accept_types[] = {GAME_START, GAME_CANCEL};
        res_msg = sw->receiveMsg(accept_types, 2);
    }
    catch (const char *error_msg)
    {
        cerr << "Could not connect to server " << host.toString();
        cerr << " : " << error_msg << endl;
        connected = false;
        return 1;
    }

    if (res_msg == NULL)
        return 1;

    if (res_msg->getType() == GAME_CANCEL)
    {
        // Game refused
        delete res_msg;
        return -1;
    }
    else
    {
        GameStartMessage *gsm = dynamic_cast<GameStartMessage *>(res_msg);
        *peerHost = gsm->getHost();
        delete gsm;
        return 0;
    }
}

int Server::replyPeerChallenge(string username, bool response, SecureHost *peerHost, uint16_t *listen_port)
{
    // TODO handle port already busy ?
    *listen_port = rand() % (TO_PORT - FROM_PORT + 1) + FROM_PORT;

    ChallengeResponseMessage msg(username, response, *listen_port);
    if (sw->sendMsg(&msg) != 0)
    {
        connected = false;
        return 1;
    }

    if (response)
    {
        Message *res_msg;
        try
        {
            MessageType accept_types[] = {GAME_START, GAME_CANCEL};
            res_msg = sw->receiveMsg(accept_types, 2);
        }
        catch (const char *error_msg)
        {
            cerr << "Could not connect to server " << host.toString();
            cerr << " : " << error_msg << endl;
            connected = false;
            return 1;
        }

        if (res_msg == NULL)
            return 1;

        if (res_msg->getType() == GAME_CANCEL)
        {
            // Game refused
            delete res_msg;
            return -1;
        }
        else
        {
            GameStartMessage *gsm = dynamic_cast<GameStartMessage *>(res_msg);
            *peerHost = gsm->getHost();
            delete gsm;
            return 0;
        }
    }
    else
    {
        return -1;
    }
}

int Server::signalGameEnd()
{
    GameEndMessage msg;
    return sw->sendMsg(&msg);
}

void Server::disconnect()
{
    connected = false;
    sw->closeSocket();
    delete sw;
    sw = NULL;
}
