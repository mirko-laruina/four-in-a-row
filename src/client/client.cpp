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

using namespace std;

#define MY_TURN    (0)
#define THEIR_TURN (1)

char players[] = {'X', 'O'};

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

int main(int argc, char** argv){
    // handle server selection on params here
    char in_buffer[256];
    int choosen_col;
    int adv_col;
    int win;
    int turn;
    SocketWrapper *sw;
    Host* peer;
    srand(time(NULL));

    printWelcome();
    cout<<endl<<"Welcome to 4-in-a-row!"<<endl;
    cout<<"The rules of the game are simple: you win when you have 4 connected tokens along any direction."<<endl;

    Connect4 c;
    cout<<"Who do you want to be? X or O ?"<<endl;

    do {
        cout<<"> ";
        cin.getline(in_buffer, sizeof(in_buffer));
    } while (!c.setPlayer(in_buffer[0]));
    cout<<"You are playing as "<<c.getPlayer()<<endl;
    

    if (argc != 1 && argc != 2 && argc != 3){
        print_help();
        return 1;
    } else if (argc == 1 || argc == 2){ // wait for peer connection
        ServerSocketWrapper *ssw;
        if (argc == 1){
            ssw = new ServerSocketWrapper();
        } else{
            ssw = new ServerSocketWrapper(atoi(argv[1]));
        }
        cout<<"Waiting for connection on port: "<<ssw->getPort()<<endl;

        sw = ssw->acceptClient();

        Host p = sw->getConnectedHost();
        peer = &p;

        cout<<"Accepted client: "<<peer->toString()<<endl;

        StartGameMessage *sgm = dynamic_cast<StartGameMessage*>(sw->receiveMsg(START_GAME));
        if (sgm == NULL){
            LOG(LOG_ERR, "Connection error");
            return 1;
        }

        LOG(LOG_INFO, "Connected to %s", peer->toString().c_str());
        turn = MY_TURN;
    } else if (argc == 3){
        peer = new Host(argv[1], atoi(argv[2]));

        cout<<"Connecting to: "<<peer->toString()<<endl;

        ClientSocketWrapper *csw = new ClientSocketWrapper();

        int ret = csw->connectServer(*peer);

        if (ret != 0){
            LOG(LOG_ERR, "Connection error");
            return 1;
        }

        sw = csw;

        StartGameMessage m;
        ret = sw->sendMsg(&m);

        if (ret != 0){
            LOG(LOG_ERR, "Connection error");
            return 1;
        }

        LOG(LOG_INFO, "Connected to %s", peer->toString().c_str());
        turn = THEIR_TURN;
    }

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
