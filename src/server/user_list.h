 /**
 * @file user_list.h
 * @author Riccardo Mancini
 * 
 * @brief Definition of the UserList class
 *
 * @date 2020-05-23
 */

#ifndef USER_LIST_H
#define USER_LIST_H

#include <pthread.h>
#include <map>
#include <cstring>

#include "config.h"
#include "user.h"

using namespace std;

class UserList{
private:
    map<string,User*> user_map_by_username;
    map<int,User*> user_map_by_fd;
    pthread_rwlock_t rwlock;
public:
    UserList();

    bool add(User *u);
    User* get(string username);
    User* get(int fd);
    void remove(string username);
    void remove(int fd);
    string listAvailableFromTo(int from);
    int size();
};

#endif // USER_LIST_H