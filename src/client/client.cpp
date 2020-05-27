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

using namespace std;

#define MY_TURN    (0)
#define THEIR_TURN (1)

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

enum UserConnectionType {CONNECT_TO_SERVER, CONNECT_TO_PEER, WAIT_FOR_PEER};
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
        } else{
            cout << "Could not parse arguments: "<< args << endl;
        }
    } while (true);    
}

int playWithPlayer(int turn, SocketWrapper *sw){
    int choosen_col, adv_col;
    int win;

    Connect4 c;
    cout<<"Who do you want to be? X or O ?"<<endl;

    do {
        cout<<"> ";
        cin.getline(in_buffer, sizeof(in_buffer));
    } while (!c.setPlayer(in_buffer[0]));
    cout<<"You are playing as "<<c.getPlayer()<<endl;

    cout<<"This is the starting board:"<<endl;
    cout<<c;

    do {
        if (turn == MY_TURN){
            cout<<"Write the column you want to insert the token to"<<endl;
            do {
                cout<<"> ";
                cin.getline(in_buffer, sizeof(in_buffer));
                choosen_col = in_buffer[0]-'0';
            } while(choosen_col < 0 || choosen_col > 7);
            
            win = c.play(choosen_col-1, c.getPlayer());
            cout<<c;
            
            if (win != -1){
                MoveMessage mm(choosen_col-1);
                int ret = sw->sendMsg(&mm);
                if (ret != 0){
                    LOG(LOG_ERR, "Connection error");
                    return 1;
                }
            }

            if(win == 1){
                cout<<"Congratulation, you won!"<<endl;
            } else if(win == -1){
                cout<<"The column is full, choose a different one!"<<endl;
                continue;
            } else if(win == -2){
                cout<<"The entire board is filled: it is a draw!"<<endl;
                break;
            } else{
                turn = THEIR_TURN;
            }
            
        } else{     // THEIR_TURN
            do {
                MoveMessage *mm;
                mm = dynamic_cast<MoveMessage*>(sw->receiveMsg(MOVE));
                if (mm == NULL){
                    LOG(LOG_ERR, "Connection error");
                    return 1;
                }
                adv_col = mm->getColumn();
                cout<<"Your enemy has chosen column "<<adv_col<<endl;
                win = c.play(adv_col, c.getAdv());
                cout<<c;
                if(win == 1){
                    cout<<"Damn! You lost!"<<endl;
                } else if(win == -1){
                    cout<<"The column is full, the adversary has lost!"<<endl;
                    break;
                } else if(win == -2){
                    cout<<"The entire board is filled: it is a draw!"<<endl;
                    break;
                }
            } while (win == -1);
            turn = MY_TURN;
        }
    } while (win == -1 || win == 0);
    return 0;
}

SocketWrapper* waitForPeer(int port){
    ServerSocketWrapper *ssw;
    ssw = new ServerSocketWrapper(port);

    cout<<"Waiting for connection on port: "<<ssw->getPort()<<endl;

    SocketWrapper *sw = ssw->acceptClient();

    Host p = sw->getConnectedHost();
    cout<<"Accepted client: "<<p.toString()<<endl;

    StartGameMessage *sgm = dynamic_cast<StartGameMessage*>(sw->receiveMsg(START_GAME));

    if (sgm == NULL){
        LOG(LOG_ERR, "Connection error");
        return NULL;
    }

    LOG(LOG_INFO, "Connected to %s", p.toString().c_str());
    return sw;
}

SocketWrapper* connectToPeer(Host peer){
    cout<<"Connecting to: "<<peer.toString()<<endl;

    ClientSocketWrapper *csw = new ClientSocketWrapper();

    int ret = csw->connectServer(peer);

    if (ret != 0){
        LOG(LOG_ERR, "Connection error");
        return NULL;
    }

    StartGameMessage m;
    ret = csw->sendMsg(&m);

    if (ret != 0){
        LOG(LOG_ERR, "Connection error");
        return NULL;
    }

    LOG(LOG_INFO, "Connected to %s", peer.toString().c_str());

    return csw;
}

int main(int argc, char** argv){
    int turn;

    SocketWrapper *sw;
    srand(time(NULL));

    printWelcome();
    cout<<endl<<"Welcome to 4-in-a-row!"<<endl;
    cout<<"The rules of the game are simple: you win when you have 4 connected tokens along any direction."<<endl;

    struct UserConnectionChoice ucc = promptChooseConnection();

    switch(ucc.connection_type){
        case WAIT_FOR_PEER:
            sw = waitForPeer(ucc.listen_port);
            turn = MY_TURN;
            break;
        case CONNECT_TO_PEER:
            sw = connectToPeer(ucc.host);
            turn = THEIR_TURN;
            break;
        case CONNECT_TO_SERVER:
            cout<<"Unimplemented"<<endl;
            exit(1);
            break;
    }

    return playWithPlayer(turn, sw);
}
