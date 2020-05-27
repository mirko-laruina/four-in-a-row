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

using namespace std;

Args::Args(char* line){
    string delimiter = " ";
    size_t pos = 0;
    string token;
    string s(line);
    list<string> token_list;
    
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        token_list.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    
    if (!s.empty()){
        token_list.push_back(s);
    }

    argc = token_list.size();
    argv = (char**) malloc(sizeof(char*) * token_list.size());
    int i = 0;
    for (list<string>::iterator it = token_list.begin(); 
            it != token_list.end(); ++it, ++i){
        argv[i] = (char*) malloc(it->size()+1);
        it->copy(argv[i], it->size());
        argv[i][it->size()] = '\0';
    }
}

Args::~Args(){
    for (int i=0; i < argc; i++){
        free(argv[i]);
    }
    free(argv);
}

ostream& operator<<(ostream& os, const Args& a){
    os << "[";

    for (int i=0; i < a.argc; i++){
        os << a.argv[i];
        if (i != a.argc-1)
            os << ",";
    }

    os << "]";
    return os;
}
