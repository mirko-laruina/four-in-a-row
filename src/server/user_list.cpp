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
#include <iostream>
#include "user_list.h"
#include "config.h"

typedef map<string,User*>::iterator Iterator;

UserList::UserList(){
    pthread_rwlock_init(&rwlock, NULL);
}

bool UserList::add(User *u){
    bool success;
    pthread_rwlock_wrlock(&rwlock);
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
    pthread_rwlock_unlock(&rwlock);

    return success;
}

User* UserList::get(string username){
    User* u = NULL;
    pthread_rwlock_rdlock(&rwlock);
    if (user_map_by_username.find(username) != user_map_by_username.end())
        u = user_map_by_username.at(username);
    pthread_rwlock_unlock(&rwlock);
    return u;
}

User* UserList::get(int fd){
    User* u = NULL;
    pthread_rwlock_rdlock(&rwlock);
    if (user_map_by_fd.find(fd) != user_map_by_fd.end())
        u = user_map_by_fd.at(fd);
    pthread_rwlock_unlock(&rwlock);
    return u;
}

void UserList::remove(string username){
    User* u;
    pthread_rwlock_wrlock(&rwlock);
    if (user_map_by_username.find(username) != user_map_by_username.end()){
        u = user_map_by_username.at(username);
        user_map_by_username.erase(username);
        user_map_by_fd.erase(u->getSocketWrapper()->getDescriptor());
    }
    pthread_rwlock_unlock(&rwlock);
}

void UserList::remove(int fd){
    User* u;
    pthread_rwlock_wrlock(&rwlock);
    if (user_map_by_fd.find(fd) != user_map_by_fd.end()){
        u = user_map_by_fd.at(fd);
        user_map_by_fd.erase(fd);
        user_map_by_username.erase(u->getUsername());
    }
    pthread_rwlock_unlock(&rwlock);
}

string UserList::listAvailableFromTo(int from){
    ostringstream os;
    int n = 0;

    pthread_rwlock_rdlock(&rwlock);
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
    pthread_rwlock_unlock(&rwlock);

    return os.str();
}

int UserList::size(){
    int sz;
    pthread_rwlock_rdlock(&rwlock);
    sz = user_map_by_fd.size();
    pthread_rwlock_unlock(&rwlock);
    return sz;
}