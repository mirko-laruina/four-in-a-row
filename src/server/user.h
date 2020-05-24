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

enum UserState {JUST_CONNECTED, AVAILABLE, CHALLENGED, PLAYING, DISCONNECTED};

class User{
private:
    string username;
    SocketWrapper *sw;
    pthread_mutex_t mutex;
    UserState state;
    string opponent_username;

    /** 
     * Number of references to this user instance
     * 
     * NB: this is updated ONLY by the UserList with its mutex
     */
    unsigned int references;
public:
    User(SocketWrapper *sw) 
            : sw(sw), state(JUST_CONNECTED), opponent_username(""), 
                references(0) {
        pthread_mutex_init(&mutex, NULL);
    }

    ~User(){
        pthread_mutex_destroy(&mutex);
        delete sw;
    }

    void lock(){pthread_mutex_lock(&mutex);}
    void unlock(){pthread_mutex_unlock(&mutex);}

    string getUsername(){return username;}
    void setUsername(string username){this->username=username;}
    SocketWrapper* getSocketWrapper(){return sw;}
    UserState getState(){return state;}
    void setState(UserState state){this->state=state;}
    string getOpponent(){return opponent_username;}
    void setOpponent(string opponent){this->opponent_username=opponent;}

    void increaseRefs(){references++;}
    void decreaseRefs(){references--;}
    int countRefs(){return references;}
};

#endif // USER_H