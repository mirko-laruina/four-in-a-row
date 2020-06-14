 /**
 * @file args.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of the Args class
 * 
 * Adapted from https://stackoverflow.com/a/14266139
 *
 * @date 2020-05-27
 */

#include "utils/args.h"
#include <sstream>

using namespace std;

void Args::parseLine(string s){
    string delimiter = " ";
    size_t pos = 0;
    string token;
    
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        argv.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    
    if (!s.empty()){
        argv.push_back(s);
    }
}

Args::Args(char* line){
    parseLine(string(line));
}

Args::Args(istream &is){
    string line;
    getline(is, line);
    if (!is){
        status = 1;
    } else {
        status = 0;
        parseLine(line);
    }
}

ostream& operator<<(ostream& os, const Args& a){
    os << "[";

    for (vector<string>::const_iterator it = a.argv.begin(); it != a.argv.end(); it++){
        os << *it;
        if (it != a.argv.end()-1)
            os << ",";
    }

    os << "]";
    return os;
}

const char* Args::c_str(){
    ostringstream os;
    os << *this;
    return os.str().c_str();
}
