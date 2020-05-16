/**
 * @file connect4.h
 * @author Mirko Laruina
 *
 * @brief Header file for the class responsible of handling the board of a Connect4 game
 *
 * @date 2020-05-14
 */

#ifndef CONNECT4_H
#define CONNECT4_H
#include <iostream>
#include <cstring>
#include "logging.h"

#define N_IN_A_ROW 4
extern const int LOG_LEVEL;

class Connect4 {
    /** Rows, cols and total size of the board */
    int rows_, cols_, size_;

    /** State of the board */
    bool full_;

    /** Matrix data structure for the board */
    char* cells_;

    /** Player marker */
    char player_;

    /** Adversary marker */
    char adversary_;

    /** 
     * Counts the tokens of the player towards a direction (di, dj) 
     * starting from  (row, col)  
     * 
     * @param player   player marker for whom we count the tokens
     * @param row      starting row
     * @param col      starting col
     * @param di       step size for rows (each iteration row+di)
     * @param dj       step size for cols (each iteration row+dj)
     *
     * @return         number of tokens along the (one way) direction
     */
    int countNexts(char player, int row, int col, int di, int dj);

    /**
     * @brief Checks if the top row is full, this would mean all the board is full
     * 
     * @return true if full
     * @return false if at least one cell is empty
     */
    bool checkFullTopRow();
    public:

    /**
     * @brief Construct a new Connect 4 object
     * 
     * @param rows      Number of rows
     * @param columns   Number of columns
     */
    Connect4(int rows = 6, int columns = 7);

    /**
     * @brief Get the number of columns of the board
     * 
     * @return number of columns 
     */
    int getNumCols();

    /**
     * Inserts a token.
     *
     * @param column   target column where the token should be added
     * @param player   player who is making the move
     * 
     * @retval 1        Success with win
     * @retval 0        Success without win
     * @retval -1       Failure for full column
     * @retval -2       Board is full, it could be so before or after the move takes place
     */
    int8_t play(int column, char player = 0);

    /**
     * Checks if an inserted token causes a win
     * 
     * @param starting_row  row of the token
     * @param starting_col  col of the token
     * @param player        marker of the player inserting the token
     *
     * @return              true if winning, false otherwise
     */
    bool checkWin(int starting_row, int starting_col, char player = 0);

    /**
     * @brief Sets the default player
     * 
     * @param player    player to be set
     * @return true if a valid player was supplied and set
     * @return false otherwise
     */
    bool setPlayer(char player);

    /**
     * @brief Get the default player
     * 
     * @return player marker
     */
    char getPlayer();

    /**
     * @brief Get the adversary, when a default player is set
     * 
     * @return enemy marker
     */
    char getAdv();

    /**
     * @brief Prints the board
     * 
     * @param os    Output stream where the board has to be printed
     */
    void print(std::ostream& os);

    friend std::ostream& operator<<(std::ostream& os, const Connect4& b);
};
#endif //CONNECT4_H