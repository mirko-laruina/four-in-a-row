/**
 * @file multi_player.h
 * @author Riccardo Mancini
 *
 * @brief Implementation of the multi player game main function and 
 *          connection with peer functions
 *
 * @date 2020-05-29
 */

#include "multi_player.h"
#include "connect4.h"
#include <iostream>

int playWithPlayer(int turn, SocketWrapper *sw){
    int choosen_col, adv_col;
    int win;
    char in_buffer[256];

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
