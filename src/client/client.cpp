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

    return 0;
}
