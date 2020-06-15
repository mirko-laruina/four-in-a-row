 /**
 * @file user_list.h
 * @author Riccardo Mancini
 * 
 * @brief Implementation of the UserList class
 *
 * @date 2020-05-23
 */

#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <iostream>
#include "user_list.h"
#include "config.h"

using namespace std;

typedef map<string,User*>::iterator Iterator;

UserList::UserList(){
    pthread_mutex_init(&mutex, NULL);
}

bool UserList::add(User *u){
    bool success;
    pthread_mutex_lock(&mutex);
    if ((success = user_map_by_fd.size() < MAX_USERS)){
        user_map_by_fd.insert(
            pair<int,User*>(u->getSocketWrapper()->getDescriptor(), u)
        );
        if (!u->getUsername().empty()){
            user_map_by_username.insert(
                pair<string,User*>(u->getUsername(), u)
            );
        }
    }
    pthread_mutex_unlock(&mutex);

    return success;
}

User* UserList::get(string username){
    User* u = NULL;
    pthread_mutex_lock(&mutex);
    if (user_map_by_username.find(username) != user_map_by_username.end()){
        u = user_map_by_username.at(username);
        u->increaseRefs();
        LOG(LOG_DEBUG, "Thread %ld got reference to user %d",
            pthread_self(), u->getSocketWrapper()->getDescriptor()
        );
    }
    pthread_mutex_unlock(&mutex);
    return u;
}

User* UserList::get(int fd){
    User* u = NULL;
    pthread_mutex_lock(&mutex);
    if (user_map_by_fd.find(fd) != user_map_by_fd.end()){
        u = user_map_by_fd.at(fd);
        u->increaseRefs();
        LOG(LOG_DEBUG, "Thread %ld got reference to user %d",
          pthread_self(), u->getSocketWrapper()->getDescriptor()
        );
    }
    pthread_mutex_unlock(&mutex);
    return u;
}

bool UserList::exists(string username){
    bool res;
    pthread_mutex_lock(&mutex);
    res = user_map_by_username.find(username) != user_map_by_username.end();
    pthread_mutex_unlock(&mutex);
    return res;
}

bool UserList::exists(int fd){
    bool res;
    pthread_mutex_lock(&mutex);
    res = user_map_by_fd.find(fd) != user_map_by_fd.end();
    pthread_mutex_unlock(&mutex);
    return res;
}

void UserList::yield(User* u){
    pthread_mutex_lock(&mutex);
    u->decreaseRefs();
    LOG(LOG_DEBUG, "Thread %ld yielded user %d (refcount: %d)",
        pthread_self(), u->getSocketWrapper()->getDescriptor(), u->countRefs()
    );
    if (u->countRefs() == 0 && u->getState() == DISCONNECTED){
        if (user_map_by_username.find(u->getUsername()) 
                != user_map_by_username.end()
        ){
            user_map_by_username.erase(u->getUsername());
        }
        if (user_map_by_fd.find(u->getSocketWrapper()->getDescriptor()) 
                != user_map_by_fd.end()
        ){
            user_map_by_fd.erase(u->getSocketWrapper()->getDescriptor());
        }
        delete u;
    }
    pthread_mutex_unlock(&mutex);
}

string UserList::listAvailableFromTo(int from){
    ostringstream os;
    int n = 0;

    pthread_mutex_lock(&mutex);
    for (Iterator it = user_map_by_username.begin(); 
        n < from+MAX_USERS_IN_MESSAGE && it != user_map_by_username.end();
        ++it
    ){
        if (it->second->getState() == AVAILABLE){
            if (n < from)
                continue;

            os << it->first;
            if (n < from+MAX_USERS_IN_MESSAGE-1)
                os << ",";
            
            n++;
        }
        
    }
    pthread_mutex_unlock(&mutex);
    os << '\0';

    return os.str();
}

int UserList::size(){
    int sz;
    pthread_mutex_lock(&mutex);
    sz = user_map_by_fd.size();
    pthread_mutex_unlock(&mutex);
    return sz;
}