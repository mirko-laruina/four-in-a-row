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
#include <vector>

using namespace std;

/**
 * Utility class that parses an input line into a list of arguments (argc, argv)
 */
class Args {
private:
    int status;
    vector<string> argv;

    void parseLine(string s);
public:
    /**
     * Default constructor
     */
    Args() : status(0) {}

    /**
     * Constructor that parses the given input line
     */
    Args(char* line);

    /**
     * Constructor that reads from the given input stream.
     */
    Args(std::istream &is);

    /**
     * Operator overload for printing the arguments with cout
     * 
     * Format: ["arg1", "arg2"]
     */
    friend std::ostream& operator<<(std::ostream& os, const Args& b);

    /**
     * Returns argument count.
     * 
     * @returns >=0 argument count
     * @retuns -1 error reading stream (may be caused by EOF)
     */
    int getArgc(){ return status == 0 ? argv.size() : -1;}

    /**
     * Returns nth argument
     */
    const char* getArgv(unsigned int i) { 
        if (status == 0 && i < argv.size()) 
            return argv.at(i).c_str();
        else
            return NULL;
    }

    /**
     * Returns user friendly content as a C string.
     */
    const char* c_str();
};

#endif //PARSE_ARGS_H