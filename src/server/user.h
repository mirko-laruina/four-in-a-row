/**
 * @file user.h
 * @author Riccardo Mancini
 * 
 * @brief Definition of the User class
 *
 * @date 2020-05-23
 */

#ifndef USER_H
#define USER_H

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <map>

#include "logging.h"
#include "network/socket_wrapper.h"
#include "network/host.h"

/** Prevent cross references between headers */
class UserList;

/** 
 * Definition of the available states the user may be in 
 * 
 * JUST_CONNECTED: the user has just_connected but not yet registered.
 * AVAILABLE: the user is registered and available for challenges.
 * CHALLENGED: the user is being challenged by or has challenged another player.
 * PLAYING: the user is currently playing with another user.
 * DISCONNECTED: the user is disconnected.
 */
enum UserState {JUST_CONNECTED, AVAILABLE, CHALLENGED, PLAYING, DISCONNECTED};

/**
 * Class representing a user.
 * 
 * Always lock() before using an instance and unlock() afterwards.
 * In order to prevent deadlocks, take locks in alphabetical order of username.
 * 
 * Limitations:
 *  - a user may be challenged by only another user at a time
 */
class User{
    friend UserList;
private:
    SocketWrapper *sw;
    UserState state;
    string username;
    string opponent_username;
    pthread_mutex_t mutex;

    /** 
     * Number of references to this user instance
     * 
     * NB: this is updated ONLY by the UserList with its mutex
     */
    unsigned int references;

    /**
     * Increases the reference count
     * 
     * Used by UserList.
     */
    void increaseRefs(){references++;}

    /**
     * Decreases the reference count
     * 
     * Used by UserList.
     */
    void decreaseRefs(){references--;}
public:
    /** 
     * Contructor 
     * 
     * The user is put in the JUST_CONNECTED STATE, username is set to empty
     * string.
     */
    User(SocketWrapper *sw) 
            : sw(sw), state(JUST_CONNECTED), 
                username(""), opponent_username(""), 
                references(0) {
        pthread_mutex_init(&mutex, NULL);
    }

    /** 
     * Destructor 
     * 
     * The socket_wrapper is deleted.
     */
    ~User(){
        pthread_mutex_destroy(&mutex);
        delete sw;
    }

    /**
     * Locks the user instance unsing the internal mutex.
     */
    void lock(){pthread_mutex_lock(&mutex);}

    /**
     * Unlocks the user instance unsing the internal mutex.
     */
    void unlock(){pthread_mutex_unlock(&mutex);}

    /**
     * Returns the username
     */
    string getUsername(){return username;}

    /**
     * Sets the username
     */
    void setUsername(string username){this->username=username;}

    /**
     * Returns the socket wrapper
     */
    SocketWrapper* getSocketWrapper(){return sw;}

    /**
     * Returns the current state of the user 
     */
    UserState getState(){return state;}

    /**
     * Sets the current state of the user
     */
    void setState(UserState state){this->state=state;}

    /**
     * Returns the username of the opponent
     */
    string getOpponent(){return opponent_username;}

    /**
     * Sets the username of the opponent
     */
    void setOpponent(string opponent){this->opponent_username=opponent;}

    /**
     * Returns the reference count
     */
    int countRefs(){return references;}
};

#endif // USER_H