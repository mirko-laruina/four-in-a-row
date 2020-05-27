 /**
 * @file args.h
 * @author Riccardo Mancini
 * 
 * @brief Definition of the Args class
 *
 * @date 2020-05-27
 */

#ifndef ARGS_H
#define ARGS_H

#include <cstring>
#include <string>
#include <list>
#include <iostream>

using namespace std;

class Args {
public:
    int argc;
    char **argv;

    Args() {}
    Args(list<string> l);
    Args(char* line);

    ~Args();

    friend std::ostream& operator<<(std::ostream& os, const Args& b);
};

#endif //PARSE_ARGS_H