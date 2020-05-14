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

    /** 
     * Counts the tokens of the player towards a direction (di, dj)
     * starting from  (row, col)  
     * 
     * @param player   player identifier for whom we count the tokens
     * @param row      starting row
     * @param col      starting col
     * @param di       step size for rows (each iteration row+di)
     * @param dj       step size for cols (each iteration row+dj)
     *
     * @return         number of tokens along the (one way) direction
     */
    int countNexts(char player, int row, int col, int di, int dj);

    public:
    Connect4(int rows = 6, int columns = 7);
    int getNumCols();

    /**
     * Inserts a token.
     *
     * @param column   target column where the token should be added
     * @param player   player who is making the move
     * 
     * @return          0 on success without win
     *                  1 on success with win
     *                  -1 on failure for full column
     *                  -2 on failure for full board
     */
    int8_t play(int column, char player = 0);
    bool checkWin(int starting_row, int starting_col, char player = 0);

    bool setPlayer(char player);
    char getPlayer();
    char getAdv();

    void print(std::ostream& os);
    friend std::ostream& operator<<(std::ostream& os, const Connect4& b);
};
#endif //CONNECT4_H