#include <iostream>
#include <cstdlib>
#include <ctime>
#include "board.h"
using namespace std;

char players[] = {'M', 'R'};

int main(){
    // handle server selection on params here
    Board b;
    int i;
    srand(time(NULL));
    while(!b.play(players[i++%2], rand()%b.getNumCols())){
        b.ugly_print(cout);
    }
    cout<<b<<"WIN"<<endl;
    return 0;
}
