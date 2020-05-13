#ifndef BOARD_H
#define BOARD_H
#include <iostream>
#include <cstring>

#define N_IN_A_ROW 4


class Board {
    int rows_, cols_, size_;
    char* cells_;

    /* 
     * Counts the tokens of the player towerds a direction (di, dj)
     * starting from  (row, col)  
     */
    int countNexts(char player, int row, int col, int di, int dj);

    public:
    Board(int rows = 6, int columns = 7);
    int getNumRows();
    bool play(char player, int column);
    bool checkWin(char player, int starting_row, int starting_col);

    void print();
    friend std::ostream& operator<<(std::ostream& os, const Board& b);
};
#endif //BOARD_H