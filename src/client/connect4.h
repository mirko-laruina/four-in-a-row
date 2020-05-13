#ifndef CONNECT4_H
#define CONNECT4_H
#include <iostream>
#include <cstring>

#define N_IN_A_ROW 4


class Connect4 {
    int rows_, cols_, size_;
    char* cells_;
    char player_;
    char adversary_;

    /* 
     * Counts the tokens of the player towards a direction (di, dj)
     * starting from  (row, col)  
     */
    int countNexts(char player, int row, int col, int di, int dj);

    public:
    Connect4(int rows = 6, int columns = 7);
    int getNumCols();
    bool play(int column, char player = 0);
    bool checkWin(int starting_row, int starting_col, char player = 0);

    bool setPlayer(char player);
    char getPlayer();
    char getAdv();

    void print(std::ostream& os);
    friend std::ostream& operator<<(std::ostream& os, const Connect4& b);
};
#endif //CONNECT4_H