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
#include "security/crypto.h"
#include "security/secure_host.h"

using namespace std;

/** Type of message length (first N bytes of packet) */
typedef uint16_t msglen_t;

#define MSGLEN_HTON(x) htons((x))
#define MSGLEN_NTOH(x) ntohs((x))

/** Utility for getting username size */
inline size_t usernameSize(string username){
    return min(username.size(), (size_t) MAX_USERNAME_LENGTH) + 1;
}

/** Utility for getting username from buffer
 * 
 * @param buf the buffer to read the string from
 * @param buflen the size of the buffer
 * @returns the read string
 */
inline string readUsername(char* buf, size_t buflen){
    size_t size = min(strnlen(buf, buflen), (size_t) MAX_USERNAME_LENGTH);
    return string(buf, size);
}

/** 
 * Utility for writing username to buffer 
 * 
 * NB: buffer must be large enough
 * 
 * @param s the username string to be written on buffer
 * @param buf the buffer to write the string to
 * @returns number of written bytes
 */
inline size_t writeUsername(string s, char* buf){
    size_t strsize = min(s.size(), (size_t)MAX_USERNAME_LENGTH);
    strncpy(buf, s.c_str(), strsize);
    buf[strsize] = '\0';
    return strsize+1;
}

/**
 * Possible type of messages.
 * 
 * When adding a new message class, add a related type here and set its
 * getType method to return it.
 */
enum MessageType
{
    SECURE_MESSAGE,
    CLIENT_HELLO,
    SERVER_HELLO,
    CLIENT_VERIFY,
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

    msglen_t size() { return usernameSize(username) + 1; }

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

    msglen_t size() { return usernameSize(username) + 1; }

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

    msglen_t size() { return usernameSize(usernames) + 1; }

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

    msglen_t size() { return usernameSize(username) + 1; }

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

    msglen_t size() { return usernameSize(username) + sizeof(response) + sizeof(listen_port) + 1; }

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

    msglen_t size() { return usernameSize(username) + 1; }

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
    X509* cert;

public:
    GameStartMessage() {}
    GameStartMessage(string username, struct sockaddr_in addr)
        : username(username), addr(addr), cert(NULL) {} //TODO cert
    ~GameStartMessage() {}

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);

    msglen_t size() { return usernameSize(username) + SERIALIZED_SOCKADDR_IN_LEN + 1; }

    string getName() { return "Game start"; }

    string getUsername() { return username; }
    struct sockaddr_in getAddr() { return addr; }
    SecureHost getHost() { return SecureHost(addr, cert); }

    MessageType getType() { return GAME_START; }
};

class SecureMessage : public Message
{
private:
    char *ct;
    msglen_t size_;
    char* tag;

public:
    SecureMessage() : ct(NULL), tag(NULL){}
    SecureMessage(char* ct, msglen_t size, char* tag) : ct(ct), size_(size), tag(tag){}
    ~SecureMessage(){ free(ct); free(tag); }

    MessageType getType() { return SECURE_MESSAGE; }
    msglen_t size() { return size_; }
    void setSize(msglen_t s) { size_ = s; }
    string getName() { return "Secure message"; }
    char* getCt() { return ct; }
    char* getTag() { return tag; }

    msglen_t write(char *buffer);
    msglen_t read(char *buffer, msglen_t len);
};

class ClientHelloMessage: public Message
{
private:
    EVP_PKEY* eph_key;
    nonce_t nonce;
    string my_id;
    string other_id;

public:
    ClientHelloMessage() : eph_key(NULL) {}
    ClientHelloMessage(EVP_PKEY* eph_key, nonce_t nonce, string my_id, string other_id) 
        : eph_key(eph_key), nonce(nonce), my_id(my_id), other_id(other_id) {}

    MessageType getType() {return CLIENT_HELLO; }
    msglen_t size() { return 1 + sizeof(nonce_t) + KEY_BIO_SIZE + usernameSize(my_id) + usernameSize(other_id); }
    string getName() { return "Client Hello message"; }
    nonce_t getNonce() { return nonce; }
    EVP_PKEY* getEphKey() { return eph_key; }
    void setEphKey(EVP_PKEY* eph_key) { this->eph_key=eph_key; }
    string getMyId() { return my_id; }
    string getOtherId() { return other_id; }

    msglen_t write(char* buffer);
    msglen_t read(char* buffer, msglen_t len);
};

class ClientVerifyMessage: public Message
{
private:
    char* ds;

public:
    ClientVerifyMessage() {}
    ClientVerifyMessage(char* ds) : ds(ds) {}
    ~ClientVerifyMessage();

    MessageType getType() {return CLIENT_VERIFY; }
    msglen_t size() { return 1 + DS_SIZE; }
    string getName() { return "Client Verify message"; }
    char* getDs() { return ds; }

    msglen_t write(char* buffer);
    msglen_t read(char* buffer, msglen_t len);
};

class ServerHelloMessage: public Message
{
private:
    EVP_PKEY* eph_key;
    nonce_t nonce;
    string my_id;
    string other_id;
    char* ds;

public:
    ServerHelloMessage(){}
    ServerHelloMessage(EVP_PKEY* eph_key, nonce_t nonce, string my_id, string other_id, char* ds) 
        : eph_key(eph_key), nonce(nonce), my_id(my_id), other_id(other_id), ds(ds) {}
    ~ServerHelloMessage();

    MessageType getType() {return SERVER_HELLO; }
    msglen_t size() { return 1 + sizeof(nonce_t) + KEY_BIO_SIZE + usernameSize(my_id) + usernameSize(other_id) + DS_SIZE; }
    string getName() { return "Server Hello message"; }
    nonce_t getNonce() { return nonce; }
    EVP_PKEY* getEphKey() { return eph_key; }
    void setEphKey(EVP_PKEY* eph_key) { this->eph_key=eph_key; }
    string getMyId() { return my_id; }
    string getOtherId() { return other_id; }
    char* getDs() { return ds; }

    msglen_t write(char* buffer);
    msglen_t read(char* buffer, msglen_t len);
};

/**
 * Reads the message using the correct class and returns a pointer to it.
 * 
 * NB: remeber to dispose of the created Message when you are done with it.
 */
Message *readMessage(char *buffer, msglen_t len);

#endif // MESSAGES_H