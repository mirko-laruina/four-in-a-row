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

#include <string>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "logging.h"
#include "config.h"
#include "network/inet_utils.h"
#include "network/host.h"

using namespace std;

/** Type of message length (first N bytes of packet) */
typedef uint16_t msglen_t;

#define MSGLEN_HTON(x) htons((x))
#define MSGLEN_NTOH(x) ntohs((x))

/**
 * Possible type of messages.
 * 
 * When adding a new message class, add a related type here and set its
 * getType method to return it.
 */
enum MessageType
{
    SECURE_MESSAGE,
    START_GAME_PEER,
    MOVE,
    REGISTER,
    CHALLENGE,
    GAME_END,
    USERS_LIST,
    USERS_LIST_REQ,
    CHALLENGE_FWD,
    CHALLENGE_RESP,
    GAME_START,
    GAME_CANCEL
};

/**
 * Abstract class for Messages.
 */
class Message
{
public:
    virtual ~Message(){};
    /** 
     * Write message to buffer
     * 
     * @returns number of bytes written
     */
    virtual msglen_t write(char *buffer) = 0;

    /** 
     * Read message from buffer
     * 
     * @returns 0 in case of success, something else in case of errors.
     *          Refer to the implementation for details
     */
    virtual msglen_t read(char *buffer, msglen_t len) = 0;

    /** 
     * Get required buffer size
     */
    virtual msglen_t size() = 0;

    /** 
     * Get message name (for debug purposes)
     */
    virtual string getName() = 0;

    virtual MessageType getType() = 0;
};

/**
 * Message that signals to start a new game.
 */
class StartGameMessage : public Message
{
public:
    StartGameMessage() {}
    ~StartGameMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return 1; }

    string getName() { return "StartGame"; }

    MessageType getType() { return START_GAME_PEER; }
};

/**
 * Message that signals a move
 */
class MoveMessage : public Message
{
private:
    char col;

public:
    MoveMessage() {}
    MoveMessage(char col) : col(col) {}
    ~MoveMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return 2; }

    string getName() { return "Move"; }

    char getColumn() { return col; }

    MessageType getType() { return MOVE; }
};

/**
 * Message that permits the client to register to server
 */
class RegisterMessage : public Message
{
private:
    string username;

public:
    RegisterMessage() {}
    RegisterMessage(string username) : username(username) {}
    ~RegisterMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)username.size(), MAX_USERNAME_LENGTH) + 1 + 1; }

    string getName() { return "Register"; }

    string getUsername() { return username; }

    MessageType getType() { return REGISTER; }
};

/**
 * Message that permits the client to challenge another client 
 * through the server.
 */
class ChallengeMessage : public Message
{
private:
    string username;

public:
    ChallengeMessage() {}
    ChallengeMessage(string username) : username(username) {}
    ~ChallengeMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)username.size(), MAX_USERNAME_LENGTH) + 1 + 1; }

    string getName() { return "Challenge"; }

    string getUsername() { return username; }

    MessageType getType() { return CHALLENGE; }
};

/**
 * Message that signals the server that the client is available
 */
class GameEndMessage : public Message
{
public:
    GameEndMessage() {}
    ~GameEndMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return 1; }

    string getName() { return "Game End"; }

    MessageType getType() { return GAME_END; }
};

/**
 * Message that the server sends the client with the list of users
 */
class UsersListMessage : public Message
{
private:
    string usernames;

public:
    UsersListMessage() {}
    UsersListMessage(string usernames) : usernames(usernames) {}
    ~UsersListMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)usernames.size(), MAX_USERNAME_LENGTH) + 1 + 1; }

    string getName() { return "User list"; }

    string getUsernames() { return usernames; }

    MessageType getType() { return USERS_LIST; }
};

/**
 * Message with which the client asks for the list of connected users.
 */
class UsersListRequestMessage : public Message
{
private:
    uint32_t offset;

public:
    UsersListRequestMessage() : offset(0) {}
    UsersListRequestMessage(unsigned int offset) : offset(offset) {}
    ~UsersListRequestMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return sizeof(offset) + 1; }

    string getName() { return "Users list request"; }

    uint32_t getOffset() { return offset; }

    MessageType getType() { return USERS_LIST_REQ; }
};

/**
 * Message with which the server forwards a challenge.
 */
class ChallengeForwardMessage : public Message
{
private:
    string username;

public:
    ChallengeForwardMessage() {}
    ChallengeForwardMessage(string username) : username(username) {}
    ~ChallengeForwardMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)username.size(), MAX_USERNAME_LENGTH) + 1 + 1; }

    string getName() { return "Challenge forward"; }

    string getUsername() { return username; }

    MessageType getType() { return CHALLENGE_FWD; }
};

/**
 * Message with which the client replies to a challenge.
 */
class ChallengeResponseMessage : public Message
{
private:
    string username;
    bool response;
    uint16_t listen_port;

public:
    ChallengeResponseMessage() {}
    ChallengeResponseMessage(string username, bool response, uint16_t port)
        : username(username), response(response), listen_port(port) {}
    ~ChallengeResponseMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)username.size(), MAX_USERNAME_LENGTH) + 1 + sizeof(response) + sizeof(listen_port) + 1; }

    string getName() { return "Challenge response"; }

    string getUsername() { return username; }
    bool getResponse() { return response; }
    uint16_t getListenPort() { return listen_port; }

    MessageType getType() { return CHALLENGE_RESP; }
};

/**
 * Message with which the server forwards a challenge rejectal or another 
 * event that caused the game to be canceled.
 */
class GameCancelMessage : public Message
{
private:
    string username;

public:
    GameCancelMessage() {}
    GameCancelMessage(string username) : username(username) {}
    ~GameCancelMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)username.size(), MAX_USERNAME_LENGTH) + 1 + 1; }

    string getName() { return "Game cancel"; }

    string getUsername() { return username; }

    MessageType getType() { return GAME_CANCEL; }
};

/**
 * Message with which the server makes a new game start between clients.
 */
class GameStartMessage : public Message
{
private:
    string username;
    struct sockaddr_in addr;

public:
    GameStartMessage() {}
    GameStartMessage(string username, struct sockaddr_in addr)
        : username(username), addr(addr) {}
    ~GameStartMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return min((int)username.size(), MAX_USERNAME_LENGTH) + 1 + SERIALIZED_SOCKADDR_IN_LEN + 1; }

    string getName() { return "Game start"; }

    string getUsername() { return username; }
    struct sockaddr_in getAddr() { return addr; }
    Host getHost() { return Host(addr); }

    MessageType getType() { return GAME_START; }
};

class SecureMessage : public Message
{
private:
    char *ct;
    int size_;
    char* tag;

public:
    MessageType getType() { return SECURE_MESSAGE; }
    msglen_t size() { return size_; }
    void setSize(int s) { size_ = s; }
    string getName() { return "Secure message"; }
    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);
};

/**
 * Reads the message using the correct class and returns a pointer to it.
 * 
 * NB: remeber to dispose of the created Message when you are done with it.
 */
Message *readMessage(char *buffer, msglen_t len);

#endif // MESSAGES_H