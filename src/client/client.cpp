/**
 * @file client.cpp
 * @author Mirko Laruina
 * 
 * @brief Implementation of a 4-in-a-row game
 *
 * @date 2020-05-14
 */
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "connect4.h"
#include "logging.h"
#include "network/socket_wrapper.h"
#include "network/host.h"
#include "utils/args.h"
#include "single_player.h"
#include "multi_player.h"

using namespace std;

static const char players[] = {'X', 'O'};
static char in_buffer[256];

/**
 * Prints command usage information.
 */
void print_help(){
  cout<<"On host A: ./client"<<endl;
  cout<<"On host B: ./client ipA portA"<<endl;
}

void printWelcome(){
    cout<<"****************************************************************\n"
        <<"*        __ __     _                                           *\n"
        <<"*       / // /    (_)___     ____ _   _________ _      __      *\n"
        <<"*      / // /_   / / __ \\   / __ `/  / ___/ __ \\ | /| / /      *\n"
        <<"*     /__  __/  / / / / /  / /_/ /  / /  / /_/ / |/ |/ /       *\n"
        <<"*       /_/    /_/_/ /_/   \\__,_/  /_/   \\____/|__/|__/        *\n"
        <<"*                                                              *\n"
        <<"****************************************************************"
        <<endl;
}

enum UserConnectionType {CONNECT_TO_SERVER, CONNECT_TO_PEER, WAIT_FOR_PEER, SINGLE_PLAYER};
struct UserConnectionChoice {
    enum UserConnectionType connection_type;
    union{
        Host host;
        int listen_port;
    };
    UserConnectionChoice(enum UserConnectionType connection_type, 
                        int listen_port) 
            : connection_type(connection_type), listen_port(listen_port) {}
    UserConnectionChoice(enum UserConnectionType connection_type, 
                        char* ip, int port) 
            : connection_type(connection_type), host(Host(ip, port)) {}

};

struct UserConnectionChoice promptChooseConnection(){
    cout<<"You can connect to a server, wait for a peer or connect to a peer"<< endl;
    cout<<"To connect to a server type: `server host port`"<< endl;
    cout<<"To connect to a peer type: `peer host port`"<< endl;
    cout<<"To wait for a peer type: `peer listen_port`"<< endl;
    cout<<"To play offline type: `offline`"<< endl;

    do {
        cout<<"> ";
        cin.getline(in_buffer, sizeof(in_buffer));
        Args args(in_buffer);
        if (args.argc == 2 && strcmp(args.argv[0], "peer") == 0){
            return UserConnectionChoice(WAIT_FOR_PEER, atoi(args.argv[1]));
        } else if (args.argc == 3 && strcmp(args.argv[0], "peer") == 0){
            return UserConnectionChoice(CONNECT_TO_PEER, args.argv[1], 
                                        atoi(args.argv[2]));
        } else if (args.argc == 3 && strcmp(args.argv[0], "server") == 0){
            return UserConnectionChoice(CONNECT_TO_SERVER, args.argv[1], 
                                        atoi(args.argv[2]));
        } else if (args.argc == 1 && strcmp(args.argv[0], "offline") == 0){
            return UserConnectionChoice(SINGLE_PLAYER, 0);
        } else if (args.argc == 0){
            cout << "Bye" << endl;
            exit(0);
        }else{
            cout << "Could not parse arguments: "<< args << endl;
        }
    } while (true);    
}


int main(int argc, char** argv){
    SocketWrapper *sw;

    printWelcome();
    cout<<endl<<"Welcome to 4-in-a-row!"<<endl;
    cout<<"The rules of the game are simple: you win when you have 4 connected tokens along any direction."<<endl;

    struct UserConnectionChoice ucc = promptChooseConnection();

    int ret;
    switch(ucc.connection_type){
        case WAIT_FOR_PEER:
            sw = waitForPeer(ucc.listen_port);
            ret = playWithPlayer(MY_TURN, sw);
            break;
        case CONNECT_TO_PEER:
            sw = connectToPeer(ucc.host);
            ret = playWithPlayer(THEIR_TURN, sw);
            break;
        case CONNECT_TO_SERVER:
            cout<<"Unimplemented"<<endl;
            ret = 1;
            break;
        case SINGLE_PLAYER:
            ret = playSinglePlayer();
            break;
    }

    return ret;
}
