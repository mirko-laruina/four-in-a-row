#include "board.h"
using namespace std;

void Board::print(){
    operator<<(cout, *this);
}

Board::Board(int rows /* = 6 */, int columns /* = 7 */){
    rows_ = rows;
    cols_ = columns;
    size_ = rows*columns;

    //Maybe check for overflow if we will use different board values

    cells_ = new char[size_];
    memset(cells_, 0, size_);
}

bool Board::play(char player, int col){
    for(int i = rows_-1; i>=0; --i){
        if(cells_[i*cols_+col] == 0){
            cells_[i*cols_+col] = player;
            return checkWin(player, i, col);
        }
    }

    //temporary, this can mean the column is full
    return false;
}

int Board::countNexts(char player, int row, int col, int di, int dj){
    int count = 0;
    for(
        int i = row+di, j = col+dj;
        i >= 0 && j >= 0 && i < rows_ && j < cols_;
        i+=di, j+=dj)
    {
        if(cells_[i*cols_+j] != player){
            break;
        } else {
            cout<<i<<" "<<j<<endl;
            count++;
        }
    }
    return count;
}

bool Board::checkWin(char player, int row, int col){
    /*
        Take any of the 4 possible directions
        count how many token of the same player there are
        before and after the new one
        if more than 4, declare win
    */

    cout<<"Checking ("<<row<<", "<<col<<")"<<endl; 

    for(int di = 1; di >= 0 && di != -1; --di){
        for(int dj = 1; dj >= 0 && di != -1; --dj){
            // direction (0, 0) is useless, since we would miss diagonal (-1, 1)
            // we can exploit the loop to iterate over that
            if(di == 0 && dj == 0){
                di = -1;
                dj = 1;
            }

            int count_forward = Board::countNexts(player, row, col, di, dj);
            int count_backward = Board::countNexts(player, row, col, -di, -dj);

            // N_IN_A_ROW minus 1 since the token just inserted is excluded
            if(count_forward + count_backward == (N_IN_A_ROW - 1)){
                return true;
            }
        }
    }

    return false;

}

int Board::getNumRows(){
    return rows_;
}

ostream& operator<<(ostream& os, const Board& b){
    for(int i = 0; i<b.rows_+2; ++i){
        os<<'-';
    }
    os<<endl;

    for(int i = 0; i<b.rows_; ++i){
        os<<'|';
        for(int j = 0; j<b.cols_; ++j){
            //cout<<"i: "<<i<<" j: "<<j<<" value: "<<b.cells_[i*b.cols_+j]<<endl;
            if(b.cells_[i*b.cols_+j] == 0){
                os<<" ";
            } else {
                os<<b.cells_[i*b.cols_+j];
            }
        }
        os<<'|'<<endl;
    }
    
    for(int i = 0; i<b.rows_+2; ++i){
        os<<'-';
    }
    os<<endl;

    return os;
}