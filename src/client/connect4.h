#ifndef CONNECT4_H
#define CONNECT4_H
#include <iostream>
#include <cstring>

#define N_IN_A_ROW 4


class Connect4 {
    int rows_, cols_, size_;
    char* cells_;

    /* 
     * Counts the tokens of the player towerds a direction (di, dj)
     * starting from  (row, col)  
     */
    int countNexts(char player, int row, int col, int di, int dj);

    public:
    Connect4(int rows = 6, int columns = 7);
    int getNumCols();
    bool play(char player, int column);
    bool checkWin(char player, int starting_row, int starting_col);

    void print(std::ostream& os);
    friend std::ostream& operator<<(std::ostream& os, const Connect4& b);
};
#endif //CONNECT4_H