/**
 * @file single_player.h
 * @author Mirko Laruina
 * @author Riccardo Mancini
 *
 * @brief Implementation of the single player game main function
 *
 * @date 2020-05-27
 */

#include "single_player.h"
#include <iostream>
#include "connect4.h"

using namespace std;

int playSinglePlayer(){
    char in_buffer[256];
    int choosen_col, adv_col;
    int win;
    Connect4 c;

    cout<<"Who do you want to be? X or O ?"<<endl;

    do {
        cout<<"> "<<flush;
        cin.getline(in_buffer, sizeof(in_buffer));
    } while (!c.setPlayer(in_buffer[0]));
    cout<<"You are playing as "<<c.getPlayer()<<endl;

    cout<<"This is the starting board:"<<endl;
    cout<<c;

    srand(time(NULL));

    do {
        cout<<"Write the column you want to insert the token to"<<endl;
        do {
            cout<<"> "<<flush;
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
                win = c.play(adv_col, c.getAdv());
                cout<<c;
                if(win == 1){
                    cout<<"Damn! You lost!"<<endl;
                } else if(win == -1){
                    cout<<"The column is full, the adversary has to chose a different one!"<<endl;
                    continue;
                } else if(win == -2){
                    cout<<"The entire board is filled: it is a draw!"<<endl;
                    break;
                }
            } while (win == -1);
        }
    } while (win == -1 || win == 0);
    return 0;
}