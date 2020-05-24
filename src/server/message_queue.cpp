 /**
 * @file message_queue.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of the MessageQueue class
 *
 * @date 2020-05-23
 */

#include "message_queue.h"


MessageQueue::MessageQueue(){
    pthread_mutex_init(&mutex, NULL);
}

bool MessageQueue::push(User* u, Message* m){
    bool success;
    pair<User*,Message*> p(u, m);
    pthread_mutex_lock(&mutex);
    if ((success = msg_queue.size() < MAX_QUEUE_LENGTH))
        msg_queue.push(p);
    pthread_mutex_unlock(&mutex);
    return success;
}

pair<User*,Message*> MessageQueue::pull(){
    pair<User*,Message*> p;
    pthread_mutex_lock(&mutex);
    p = msg_queue.front();
    msg_queue.pop();
    pthread_mutex_unlock(&mutex);
    return p;
}

pair<User*,Message*> MessageQueue::pullWait(){
    pair<User*,Message*> p;
    pthread_mutex_lock(&mutex);
    while(msg_queue.empty())
        pthread_cond_wait(&available_messages, &mutex);
    p = msg_queue.front();
    msg_queue.pop();
    pthread_mutex_unlock(&mutex);
    return p;
}

bool MessageQueue::pushSignal(User* u, Message* m){
    bool success;
    pair<User*,Message*> p(u, m);
    pthread_mutex_lock(&mutex);
    if ((success = msg_queue.size() < MAX_QUEUE_LENGTH)){
        msg_queue.push(p);
        if (msg_queue.size() == 1)
            pthread_cond_signal(&available_messages);
    }
    pthread_mutex_unlock(&mutex);
    return success;

}
