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

Server::~Server(){
    sw->closeSocket();
    delete sw;
}

int Server::registerToServer(string username){
    if (sw->connectServer(host) != 0)
        return 1;

    RegisterMessage msg(username);

    return sw->sendMsg(&msg);
}

string Server::getUserList(){
    UsersListRequestMessage req_msg;

    if (sw->sendMsg(&req_msg) != 0){
        return "";
    }

    UsersListMessage* res_msg;

    try{
        res_msg = dynamic_cast<UsersListMessage*>(sw->receiveMsg(USERS_LIST));
    } catch (const char* error_msg){
        cerr<<"Could not connect to server " <<host.toString();
        cerr<<" : "<<error_msg << endl;
        return "";
    }
    
    string usernames = res_msg->getUsernames();
    delete res_msg;

    return usernames;
}

int Server::challengePeer(string username, Host* peerHost){
    ChallengeMessage req_msg(username);
    
    if (sw->sendMsg(&req_msg) != 0)
        return 1;

    Message* res_msg;

    try{
        MessageType accept_types[]={GAME_START, GAME_CANCEL};
        res_msg = sw->receiveMsg(accept_types, 2);
    } catch (const char* error_msg){
        cerr<<"Could not connect to server " <<host.toString();
        cerr<<" : "<<error_msg << endl;
        return 1;
    }

    if (res_msg->getType() == GAME_CANCEL){
        // Game refused
        delete res_msg;
        return -1;
    } else {
        GameStartMessage* gsm = dynamic_cast<GameStartMessage*>(res_msg);
        *peerHost = gsm->getHost();
        delete gsm;
        return 0;
    }   
}

int Server::replyPeerChallenge(string username, bool response, Host* peerHost, uint16_t *listen_port){
    // TODO handle port already busy
    *listen_port = rand() % (TO_PORT - FROM_PORT + 1) + FROM_PORT;

    ChallengeResponseMessage msg(username, response, *listen_port);
    if(sw->sendMsg(&msg) != 0){
        return 1;
    }

    if (response){
        Message* res_msg;
        try{
            MessageType accept_types[]={GAME_START, GAME_CANCEL};
            res_msg = sw->receiveMsg(accept_types, 2);
        } catch (const char* error_msg){
            cerr<<"Could not connect to server " <<host.toString();
            cerr<<" : "<<error_msg << endl;
            return 1;
        }

        if (res_msg->getType() == GAME_CANCEL){
            // Game refused
            delete res_msg;
            return -1;
        } else {
            GameStartMessage* gsm = dynamic_cast<GameStartMessage*>(res_msg);
            *peerHost = gsm->getHost();
            delete gsm;
            return 0;
        }   
    } else{
        return -1;
    }
}
