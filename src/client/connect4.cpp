/**
 * @file connect4.cpp
 * @author Mirko Laruina
 *
 * @brief Implementation of connect4.h
 *
 * @date 2020-05-14
 *
 * @see connect4.h
 * 
 */
#include "connect4.h"
using namespace std;

void Connect4::print(ostream& os){
    os<<*this;
}

Connect4::Connect4(int rows /* = 6 */, int columns /* = 7 */){
    rows_ = rows;
    cols_ = columns;
    size_ = rows*columns;
    full_ = false;

    //Maybe check for overflow if we will use different board values

    cells_ = new char[size_];
    memset(cells_, 0, size_);
}

int8_t Connect4::play(int col, char player){
    // bool col_full = true;
    if(player == 0){
        player = player_;
    }

    //Trying to play with a full board
    if(full_){
        return -2;
    }

    for(int i = rows_-1; i>=0; --i){
        if(cells_[i*cols_+col] == 0){
            // col_full = false;
            cells_[i*cols_+col] = player;
            if( checkWin(i, col, player) ){
                return 1;
            } else {
                //All the board could be full now
                if(i == 0 && checkFullTopRow()){
                    full_ = true;
                    return -2;
                } else {
                    return 0;
                }
            }
        }
    }

    //We are sure the board is not full, otherwise we would have already exited
    //If a play was possible, we would have already exited too
    //Only possible case is full column
    return -1;
}

int Connect4::countNexts(char player, int row, int col, int di, int dj){
    int count = 0;
    for(
        int i = row+di, j = col+dj;
        i >= 0 && j >= 0 && i < rows_ && j < cols_;
        i+=di, j+=dj)
    {
        if(cells_[i*cols_+j] != player){
            break;
        } else {
            LOG(LOG_DEBUG, "%d %d", i, j);
            count++;
        }
    }
    return count;
}

bool Connect4::checkWin(int row, int col, char player){
    /*
        Take any of the 4 possible directions
        count how many token of the same player there are
        before and after the new one
        if more than 4, declare win
    */

    LOG(LOG_DEBUG, "Checking (%d, %d)", row, col); 
    if(player == 0){
        player = player_;
    }

    for(int di = 1; di >= 0 && di != -1; --di){
        for(int dj = 1; dj >= 0 && di != -1; --dj){
            // direction (0, 0) is useless, since we would miss diagonal (-1, 1)
            // we can exploit the loop to iterate over that
            if(di == 0 && dj == 0){
                di = -1;
                dj = 1;
            }

            int count_forward = Connect4::countNexts(player, row, col, di, dj);
            int count_backward = Connect4::countNexts(player, row, col, -di, -dj);

            // N_IN_A_ROW minus 1 since the token just inserted is excluded
            if(count_forward + count_backward >= (N_IN_A_ROW - 1)){
                return true;
            }
        }
    }

    return false;

}

bool Connect4::checkFullTopRow(){
    for(int j = 0; j<cols_; ++j){
        if(cells_[j] == 0){
            return false;
        }
    }
    return true;
}

int Connect4::getNumCols(){
    return cols_;
}

bool Connect4::setPlayer(char player){
    if(player == 'X' || player == 'x'
        || player == 'O' || player == 'o'){
        player_ = toupper(player);
        adversary_ = player_ == 'X' ? 'O' : 'X';
        return true;
    }
    return false;
}

char Connect4::getPlayer(){
    return player_;
}

char Connect4::getAdv(){
    return adversary_;
}

ostream& operator<<(ostream& os, const Connect4& c){
    int width = 2+3*(c.rows_+1);
    for(int i = 0; i<width; ++i){
        os<<'*';
    }
    os<<endl;

    for(int i = 0; i<c.rows_; ++i){
        os<<"*";
        for(int j = 0; j<c.cols_; ++j){
            if(c.cells_[i*c.cols_+j] == 0){
                os<<"   ";
            } else {
                os<< " " << (c.cells_[i*c.cols_+j] == 'X' ? "\033[31mX" : "\033[34mO") <<" ";
            }
        }
        os<<"\033[0m*"<<endl;
    }
    
    for(int i = 0; i<width; ++i){
        os<<'*';
    }
    os<<endl;

    for(int i = 0; i<width; ++i){
        if( (i+1)%3 == 0  ){
            os<<(i+1)/3;
        } else {
            os<<" ";
        }
    }
    os<<endl;
    return os;
}