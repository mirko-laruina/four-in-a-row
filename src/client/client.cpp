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
#include "connection_mode.h"
#include "server_lobby.h"
#include "security/crypto.h"

using namespace std;

static const char players[] = {'X', 'O'};
static char in_buffer[256];

/**
 * Prints command usage information.
 */
void print_help(char* argv0){
  cout<<"Usage: "<<argv0<<" cert.pem key.pem cacert.pem crl.pem [other_cert.pem]"<<endl;
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

struct ConnectionMode promptChooseConnection(){
    cout<<"You can connect to a server, wait for a peer or connect to a peer"<< endl;
    cout<<"To connect to a server type: `server host port path/to/server_cert.pem`"<< endl;
    cout<<"To connect to a peer type: `peer host port path/to/peer_cert.pem`"<< endl;
    cout<<"To wait for a peer type: `peer listen_port path/to/peer_cert.pem`"<< endl;
    cout<<"To play offline type: `offline`"<< endl;

    do {
        cout<<"> "<<flush;
        cin.getline(in_buffer, sizeof(in_buffer));
        Args args(in_buffer);
        if (args.argc == 2 && strcmp(args.argv[0], "peer") == 0){
            return ConnectionMode(WAIT_FOR_PEER, atoi(args.argv[1]));
        } else if (args.argc == 4 && strcmp(args.argv[0], "peer") == 0){
            X509* cert = load_cert_file(args.argv[3]);
            return ConnectionMode(CONNECT_TO_PEER, args.argv[1], 
                                        atoi(args.argv[2]), cert);
        } else if (args.argc == 4 && strcmp(args.argv[0], "server") == 0){
            X509* cert = load_cert_file(args.argv[3]);
            return ConnectionMode(CONNECT_TO_SERVER, args.argv[1], 
                                        atoi(args.argv[2]), cert);
        } else if (args.argc == 1 && strcmp(args.argv[0], "offline") == 0){
            return ConnectionMode(SINGLE_PLAYER, 0);
        } else if (args.argc == 0){
            cout << "Bye" << endl;
            exit(0);
        }else{
            cout << "Could not parse arguments: "<< args << endl;
        }
    } while (true);    
}


int main(int argc, char** argv){
    SecureSocketWrapper *sw;

    if (argc < 5){
        print_help(argv[0]);
        return 1;
    }

    X509* cert = load_cert_file(argv[1]);
    EVP_PKEY* key = load_key_file(argv[2], NULL);
    X509* cacert = load_cert_file(argv[3]);
    X509_CRL* crl = load_crl_file(argv[4]);
    X509_STORE* store = build_store(cacert, crl);

    srand(time(NULL));

    printWelcome();
    cout<<endl<<"Welcome to 4-in-a-row!"<<endl;
    cout<<"The rules of the game are simple: you win when you have 4 connected tokens along any direction."<<endl;

    struct ConnectionMode ucc = promptChooseConnection();

    if (ucc.connection_type == CONNECT_TO_SERVER){
        ucc = serverLobby(ucc.host, cert, key, store);
    }

    int ret;
    switch(ucc.connection_type){
        case WAIT_FOR_PEER:
            sw = waitForPeer(ucc.listen_port, cert, key, store);
            if (sw != NULL)
                ret = playWithPlayer(MY_TURN, sw);
            else 
                ret = 1;
            break;
        case CONNECT_TO_PEER:
            sw = connectToPeer(ucc.host, cert, key, store);
            if (sw != NULL)
                ret = playWithPlayer(THEIR_TURN, sw);
            else 
                ret = 1;
            break;
        case SINGLE_PLAYER:
            ret = playSinglePlayer();
            break;
        case EXIT:
            ret = ucc.exit_code;
            break;
        case CONNECT_TO_SERVER:
            ret = 1;
            break;
    }

    return ret;
}
