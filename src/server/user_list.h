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

/**
 * Class that manages the users.
 * 
 * 1) keeps track of connected users through two hash maps (one by username) and
 *    one by file descriptor of the socket the user is connected to.
 * 2) updates reference count of the users in order to safely delete 
 *    disconnected users only once no thread holds a reference to it.
 * 3) disposes of disconnected users that are no longer referenced by any   
 *    thread.
 * 
 * Every method in this class is protected against concurrent modifications by
 * a mutex.
 * 
 * The maximum number of users is defined by MAX_USERS.
 */
class UserList{
private:
    map<string,User*> user_map_by_username;
    map<int,User*> user_map_by_fd;
    pthread_mutex_t mutex;
public:
    /**
     * Initializes an empty list.
     */
    UserList();

    /**
     * Inserts a new user to the internal hash maps.
     * 
     * The user may not have a username but must have a file descriptor.
     * 
     * Calling this function does not increase the reference count of user since
     * it's assumed that either the reference is already held or that the user
     * will no longer be needed by the calling thread.
     * In case this is not true, call also one of the get functions to correctly
     * update the reference count.
     * 
     * @param u the user to be added
     * @return true in case of success, false otherwise (e.g. full)
     */
    bool add(User *u);

    /**
     * Returns the user matching the given username.
     * 
     * The reference count of the user is increased.
     * 
     * @param username the username of the user to be retrieved
     * @return a pointer to the requested user or NULL in case it is not found.
     */
    User* get(string username);

    /**
     * Returns the user matching the given file descriptor.
     * 
     * The reference count of the user is increased.
     * 
     * @param fd the file descriptor of the user to be retrieved
     * @return a pointer to the requested user or NULL in case it is not found.
     */
    User* get(int fd);

    /**
     * Checks whether there is a user matching the given username.
     * 
     * @param username the username to be checked
     * @return true if exists, false otherwise
     */
    bool exists(string username);

    /**
     * Checks whether there is a user matching the given file descriptor.
     * 
     * @param fd the file descriptor to be checked
     * @return true if exists, false otherwise
     */
    bool exists(int fd);

    /**
     * Signals that the given user is no longer being used.
     * 
     * The reference count of the user is decreased. If the reference count is 
     * 0 and the user is disconnected, the user is deleted.
     * 
     * NB: the deletion of a user may put sockets in an inconsistent state that
     *     must be fixed by the user of this class
     * 
     * @param u the user whose reference is being marked as no longer used
     */
    void yield(User *u);

    /**
     * Returns a comma separated list of users in the AVAILABLE state, starting
     * from the given offset.
     * 
     * @param from the offset
     * @return the comma separated list of users as a string
     */
    string listAvailableFromTo(int from);

    /**
     * Returns the number of all users in the list.
     */
    int size();
};

#endif // USER_LIST_H