#ifndef BOARD_H
#define BOARD_H
#include <iostream>
#include <cstring>


class Board {
    int rows_, cols_, size_;
    char* cells_;

    public:
    Board(int rows = 6, int columns = 7);
    int getNumRows();
    bool play(char player, int column);
    bool checkWin(char player, int starting_row, int starting_col);

    void print();
    friend std::ostream& operator<<(std::ostream& os, const Board& b);
};
#endif //BOARD_H