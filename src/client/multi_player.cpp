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
#include "utils/args.h"

int playWithPlayer(int turn, SecureSocketWrapper *sw){
    int choosen_col, adv_col;
    int win;

    Connect4 c;
    cout<<"Who do you want to be? X or O ?"<<endl;

    do {
        cout<<"> "<<flush;
        Args args(cin);
        if (args.getArgc() == -1){
            return 1;
        } else if (args.getArgc() == 1 && c.setPlayer(args.getArgv(0)[0])){
            break;
        } else if (args.getArgc() < 0) {
            return 1;
        } else{
            continue;
        }
    } while (1);

    cout<<"You are playing as "<<c.getPlayer()<<endl;

    cout<<"This is the starting board:"<<endl;
    cout<<c;

    do {
        if (turn == MY_TURN){
            cout<<"Write the column you want to insert the token to"<<endl;
            do {
                cout<<"> "<<flush;
                Args args(cin);
                if (args.getArgc() == 1){
                    choosen_col = args.getArgv(0)[0]-'0';
                } else if (args.getArgc() < 0){
                    return 1;
                } else {
                    choosen_col = -1;
                }
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

SecureSocketWrapper* waitForPeer(int port, SecureHost host, X509* cert, EVP_PKEY* key, X509_STORE* store){
    int ret;
    
    ServerSecureSocketWrapper *ssw;
    ssw = new ServerSecureSocketWrapper(cert, key, store);

    
    ret = ssw->bindPort(port);
    if (ret != 0){
        cout<<"Could not bind to port: "<<ssw->getPort()<<endl;
        delete ssw;
        return NULL;
    }

    cout<<"Waiting for connection on port: "<<ssw->getPort()<<endl;

    SecureSocketWrapper *sw = ssw->acceptClient(host.getCert());

    if (sw == NULL){
        LOG(LOG_ERR, "Connection error: no client with valid certificate connected");
        return NULL;
    }

    ret = sw->handshakeServer();

    if (ret != 0){
        LOG(LOG_ERR, "Handshake error");
        return NULL;
    }

    Host p = sw->getConnectedHost();
    cout<<"Accepted client: "<<p.toString()<<endl;

    StartGameMessage *sgm = dynamic_cast<StartGameMessage*>(sw->receiveMsg(START_GAME_PEER));

    if (sgm == NULL){
        LOG(LOG_ERR, "Connection error");
        return NULL;
    }

    LOG(LOG_INFO, "Connected to %s", p.toString().c_str());
    return sw;
}

SecureSocketWrapper* connectToPeer(SecureHost peer, X509* cert, EVP_PKEY* key, X509_STORE* store){
    cout<<"Connecting to: "<<peer.toString()<<endl;

    ClientSecureSocketWrapper *csw = new ClientSecureSocketWrapper(cert, key, store);

    int ret;
    int retry = 10;
    do {
        retry--;
        ret = csw->connectServer(peer); 
        if (ret != 0){
            if (retry != 0){
                LOG(LOG_INFO, "Peer is not online yet, retrying in 1 second.");
                sleep(1);
            } else {
                break;
            }
        }
    } while(ret != 0);

    if (ret != 0){
        LOG(LOG_ERR, "Connection error");
        return NULL;
    }

    ret = csw->handshakeClient();

    if (ret != 0){
        LOG(LOG_ERR, "Handshake error");
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
