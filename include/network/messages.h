/**
 * @file messages.h
 * @author Riccardo Mancini
 *
 * @brief Definition of messages
 *
 * @date 2020-05-17
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include "logging.h"
#include <string>

using namespace std;

/** 
 * Maximum message size.
 * 
 * TODO: it is random, calculate it
 */
#define MAX_MSG_SIZE 1024

/**
 * Possible type of messages.
 * 
 * When adding a new message class, add a related type here and set its
 * getType method to return it.
 */
enum MessageType {START_GAME, MOVE};

/**
 * Abstract class for Messages.
 */
class Message{
public:
    /** 
     * Write message to buffer
     * 
     * @returns number of bytes written
     */
    virtual int write(char *buffer) = 0;

    /** 
     * Read message from buffer
     * 
     * @returns 0 in case of success, something else in case of errors.
     *          Refer to the implementation for details
     */
    virtual int read(char *buffer, int len) = 0;

    /** 
     * Get required buffer size
     */
    virtual size_t size() = 0;

    /** 
     * Get message name (for debug purposes)
     */
    virtual string getName() = 0;

    virtual MessageType getType() = 0;
};

/**
 * Message that signals to start a new game.
 */
class StartGameMessage : public Message{
public:
    StartGameMessage() {}

    int write(char *buffer);
    int read(char *buffer, int len);

    size_t size(){return 1;}

    string getName(){return "StartGame";}

    MessageType getType(){return START_GAME;}
};

/**
 * Message that signals a move
 */
class MoveMessage : public Message{
private:
    char col;
public:
    MoveMessage(){}
    MoveMessage(char col) : col(col) {}

    int write(char *buffer);
    int read(char *buffer, int len);

    size_t size(){return 2;}

    string getName(){return "Move";}

    char getColumn(){return col;}

    MessageType getType(){return MOVE;}
};

/**
 * Reads the message using the correct class and returns a pointer to it.
 * 
 * NB: remeber to dispose of the created Message when you are done with it.
 */
Message* readMessage(char *buffer, int len);

#endif // MESSAGES_H