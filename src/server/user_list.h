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
#include <list>

#include "config.h"
#include "user.h"

using namespace std;

class UserList{
private:
    map<string,User*> user_map_by_username;
    map<int,User*> user_map_by_fd;
    pthread_mutex_t mutex;
public:
    UserList();

    bool add(User *u);
    User* get(string username);
    User* get(int fd);
    bool exists(string username);
    bool exists(int fd);
    void yield(User *u);
    string listAvailableFromTo(int from);
    int size();
};

#endif // USER_LIST_H