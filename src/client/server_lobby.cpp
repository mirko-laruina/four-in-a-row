/**
 * @file server_lobby.cpp
 * @author Riccardo Mancini
 *
 * @brief Implementation of the function that handles user and network input 
 *          while the user is in the server lobby waiting for a game to start
 * 
 * The select implementation was inspired by
 * https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 * 
 * @date 2020-05-29
 */

#include <sys/select.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <sstream>

#include "utils/args.h"
#include "network/socket_wrapper.h"

#include "server_lobby.h"
#include "server.h"

using namespace std;

/** stdin has file descriptor 0 in Unix */
#define STDIN (0)

void printAvailableActions(){
    cout<<"You can list users, challenge a user, exit or simply wait for other users to challenge you."<< endl;
    cout<<"To list users type: `list`"<< endl;
    cout<<"To challenge a user type: `challenge username`"<< endl;
    cout<<"To exit type: `exit`"<< endl;
    cout<<"NB: you cannot receive challenges if you are challenging another user"<< endl;
}

int doAction(Args args, Server *server, Host* peer_host){
    ostringstream os;
    os << args;
    LOG(LOG_DEBUG, "Args: %s", os.str().c_str());
    if (args.argc == 1 && strcmp(args.argv[0], "exit") == 0){
        return -2;
    } else if (args.argc == 1 && strcmp(args.argv[0], "list") == 0){
        cout<<"Retrieving the list of users..."<<endl;
        string userlist = server->getUserList(); // TODO error management
        cout<<"Online users: "<<userlist<<endl;
        return 0;
    } else if (args.argc == 2 && strcmp(args.argv[0], "challenge") == 0){
        cout<<"Sending challenge to "<<args.argv[1]<<" and waiting for response..."<<endl;
        string username(args.argv[1]);
        int ret = server->challengePeer(username, peer_host);
        switch (ret){ 
            case -1: // refused
                cout<<username<<" refused your challenge"<<endl;
                return 0;
            case 0: //accepted
                cout<<username<<" accepted your challenge"<<endl;
                return -1;
            default:
                cout<<"Error connecting to server!"<<endl;
                return 1;
        } 
    } else{
        return 0;
    }

}

int handleReceivedChallenge(Server *server, 
                            ChallengeForwardMessage* msg, 
                            Host* peer_host,
                            uint16_t* listen_port){
    char in_buffer[2];
    cout<<endl<<"You received a challenge from "<<msg->getUsername()<<endl;
    cout<<"Do you want to accept? (y/n)";
    do{
        cout<<"> "<<flush;
        cin.getline(in_buffer, sizeof(in_buffer));
    } while(strlen(in_buffer) != 0 && strcmp(in_buffer, "y") != 0 && strcmp(in_buffer, "n") != 0);

    bool response = strcmp(in_buffer, "y") == 0;

    return server->replyPeerChallenge(msg->getUsername(), response, peer_host, listen_port);
}


ConnectionMode serverLobby(Host host){
    fd_set active_fd_set, read_fd_set;

    Server server(host);

    char in_buffer[256];

    cout<<"What is your username?"<<endl;

    do{
        cout<<"> "<<flush;
        cin.getline(in_buffer, sizeof(in_buffer));
        if (strlen(in_buffer) < MIN_USERNAME_LENGTH){
            cout<<"Username must be at least "<<MIN_USERNAME_LENGTH<<" characters."<<endl;
        }
        if (strlen(in_buffer) > MAX_USERNAME_LENGTH){
            cout<<"Username must be at most "<<MAX_USERNAME_LENGTH<<" characters."<<endl;
        }
    } while(strlen(in_buffer) < MIN_USERNAME_LENGTH 
            || strlen(in_buffer) > MAX_USERNAME_LENGTH);

    string username(in_buffer);

    cout<<"Registering to "<<host.toString()<<" as "<<username<<endl;
    
    if (server.registerToServer(username) != 0){
        cout<<"Connection to "<<host.toString()<<" failed!"<<endl;
        return ConnectionMode(EXIT, -1);
    }

    Host peer_host;

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(server.getSocketWrapper()->getDescriptor(), &active_fd_set);
    FD_SET(STDIN, &active_fd_set);

    printAvailableActions();

    while (1){
        cout<<endl<<"> "<<flush;

        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        /* Service all the sockets with input pending. */
        for (int i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET(i, &read_fd_set)){
                if (i == server.getSocketWrapper()->getDescriptor()){
                    // Message from server.
                    Message* msg;
                    try{
                        msg = server.getSocketWrapper()->receiveAnyMsg();
                    } catch(const char* msg){
                        LOG(LOG_ERR, "Error: %s", msg);
                        return ConnectionMode(EXIT,1);
                    }

                    ChallengeForwardMessage* cfm;

                    int ret;
                    LOG(LOG_DEBUG, "Server sent message %s", msg->getName().c_str());
                    switch(msg->getType()){
                        case CHALLENGE_FWD:
                            uint16_t listen_port;
                            cfm = dynamic_cast<ChallengeForwardMessage*>(msg);
                            ret = handleReceivedChallenge(&server, cfm, &peer_host, &listen_port);
                            switch (ret){
                                case -1: // game canceled
                                    cout<<"Game was canceled"<<endl;
                                    break;
                                case 0:
                                    cout<<"Starting game..."<<endl;
                                    return ConnectionMode(WAIT_FOR_PEER, listen_port);
                                default:
                                    cout<<"Error"<<endl; 
                                    return ConnectionMode(EXIT, 1);
                            }
                            break;
                        default:
                            // other messages are handled internally to 
                            // Server since they require the user to wait
                            LOG(LOG_WARN, "Received unexpected message %s", msg->getName().c_str());
                    }

                } else if (i == STDIN){
                    int ret;
                    // Input from user
                    cin.getline(in_buffer, sizeof(in_buffer));
                    if (strlen(in_buffer) == 0){
                        ret = -2; // received EOF
                    } else{
                        ret = doAction(Args(in_buffer), &server, &peer_host);
                    }
                    switch (ret){
                        case 0: // do nothing
                            LOG(LOG_DEBUG, "No action");
                            break;
                        case 1: // error
                            cout<<"Error!"<<endl;
                            return ConnectionMode(EXIT, 1);
                        case -1: // challenge accepted
                            cout<<"Starting game..."<<endl;
                            return ConnectionMode(CONNECT_TO_PEER, peer_host);
                        case -2:
                            cout<<"Bye"<<endl;
                            return ConnectionMode(EXIT, 0);
                    default:
                        break;
                    }
                }
            }
        }
    }
}

