 /**
 * @file message_queue.h
 * @author Riccardo Mancini
 * 
 * @brief Definition and implementation of the MessageQueue class
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

#include "config.h"

using namespace std;

template <typename T>
class MessageQueue{
private:
    queue<T> msg_queue;
    pthread_mutex_t mutex;
    pthread_cond_t available_messages;
public:
    MessageQueue();
    bool push(T e);
    bool pushSignal(T e);
    T pull();
    T pullWait();
};

// implementation must stay in header since I've used a template

template <typename T>
MessageQueue<T>::MessageQueue(){
    pthread_mutex_init(&mutex, NULL);
}

template <typename T>
bool MessageQueue<T>::push(T e){
    bool success;
    pthread_mutex_lock(&mutex);
    if ((success = msg_queue.size() < MAX_QUEUE_LENGTH))
        msg_queue.push(e);
    pthread_mutex_unlock(&mutex);
    return success;
}

template <typename T>
T MessageQueue<T>::pull(){
    T e;
    pthread_mutex_lock(&mutex);
    e = msg_queue.front();
    msg_queue.pop();
    pthread_mutex_unlock(&mutex);
    return e;
}

template <typename T>
T MessageQueue<T>::pullWait(){
    T e;
    pthread_mutex_lock(&mutex);
    while(msg_queue.empty())
        pthread_cond_wait(&available_messages, &mutex);
    e = msg_queue.front();
    msg_queue.pop();
    pthread_mutex_unlock(&mutex);
    return e;
}

template <typename T>
bool MessageQueue<T>::pushSignal(T e){
    bool success;
    pthread_mutex_lock(&mutex);
    if ((success = msg_queue.size() < MAX_QUEUE_LENGTH)){
        msg_queue.push(e);
        if (msg_queue.size() == 1)
            pthread_cond_signal(&available_messages);
    }
    pthread_mutex_unlock(&mutex);
    return success;

}

#endif // MESSAGE_QUEUE_H
