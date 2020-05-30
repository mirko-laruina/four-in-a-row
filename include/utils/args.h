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

/**
 * Utility class that parses an input line into a list of arguments (argc, argv)
 */
class Args {
public:
    int argc;
    char **argv;

    /**
     * Default constructor
     */
    Args() {}

    /**
     * Convert a list of strings into the argc,argv format
     */
    Args(list<string> l);

    /**
     * Constructor that parses the given input line
     */
    Args(char* line);

    /**
     * Frees memory occupied by argv items and argv itself.
     */
    ~Args();

    /**
     * Operator overload for printing the arguments with cout
     * 
     * Format: ["arg1", "arg2"]
     */
    friend std::ostream& operator<<(std::ostream& os, const Args& b);
};

#endif //PARSE_ARGS_H