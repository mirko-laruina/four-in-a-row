 /**
 * @file message_queue.h
 * @author Riccardo Mancini
 * 
 * @brief Definition of the MessageQueue class
 *
 * @date 2020-05-23
 */

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <queue>
#include <utility>

#include "network/messages.h"
#include "config.h"
#include "user.h"


class MessageQueue{
private:
    queue<pair<User*,Message*>> msg_queue;
    pthread_mutex_t mutex;
    pthread_cond_t available_messages;
public:
    MessageQueue();
    bool push(User* u, Message* m);
    pair<User*,Message*> pull();
    pair<User*,Message*> pullWait();
    bool pushSignal(User* u, Message* m);
};

#endif // MESSAGE_QUEUE_H
