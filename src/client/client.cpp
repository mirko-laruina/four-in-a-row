#include <iostream>
#include <cstdlib>
#include <ctime>
#include "connect4.h"
using namespace std;

char players[] = {'X', 'O'};

int main(){
    // handle server selection on params here
    Connect4 c;
    int i;
    srand(time(NULL));
    while(!c.play(players[i++%2], rand()%c.getNumCols())){
        c.print(cout);
    }
    cout<<c<<"WIN"<<endl;
    return 0;
}
