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
using namespace std;

char players[] = {'X', 'O'};

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

int main(){
    // handle server selection on params here
    char in_buffer[256];
    int choosen_col;
    int adv_col;
    int win;
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
    cout<<"This is the starting board:"<<endl;
    cout<<c;

    do {
        cout<<"NOTE: the game may never end"<<endl;
        cout<<"Write the column you want to insert the token to"<<endl;
        do {
            cout<<"> ";
            cin.getline(in_buffer, sizeof(in_buffer));
            choosen_col = in_buffer[0]-'0';
        } while(choosen_col < 0 || choosen_col > 7);
        
        win = c.play(choosen_col-1, c.getPlayer());
        cout<<c;
        if(win == 1){
            cout<<"Congratulation, you won!"<<endl;
        } else if(win == -1){
            cout<<"The column is full, choose a different one!"<<endl;
            continue;
        } else if(win == -2){
            cout<<"The entire board is filled: it is a draw!"<<endl;
            break;
        }

        if(win != 1){
            do {
                adv_col = rand()%c.getNumCols();
                cout<<"Your enemy has chosen column "<<adv_col<<endl;
                win = c.play(rand()%c.getNumCols(), c.getAdv());
                cout<<c;
                if(win == 1){
                    cout<<"Damn! You lost!"<<endl;
                } else if(win == -1){
                    cout<<"The column is full, the adversary has to chose a different one!"<<endl;
                    continue;
                } else if(win == -2){
                    cout<<"The entire board is filled: it is a draw!"<<endl;
                }
            } while (win == -1);
        }
    } while (win == -1 || win == 0);
    return 0;
}
