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

/**
 * Thread-safe message queue template
 * 
 * Threads are blocked on a pthread_cond if no message is available and are 
 * awaken by a pthread_cond_signal when a new item is added. Access to the class
 * is regulated by a mutex.
 */
template <typename T, int MAX_SIZE>
class MessageQueue{
private:
    queue<T> msg_queue;
    pthread_mutex_t mutex;
    pthread_cond_t available_messages;
public:
    /**
     * Default constructor that creates an empty queue and initializes both 
     * mutex and cond
     */
    MessageQueue();

    /**
     * Insert a new element to the back of the queue WITHOUT SIGNALING on cond.
     * 
     * @param T the element to be inserted
     * @return true if insertion was successfull, false otherwise
     */
    bool push(T e);

    /**
     * Insert a new element to the back of the queue, signaling any blocked
     * thread that new items are available.
     * 
     * @param T the element to be inserted
     * @return true if insertion was successfull, false otherwise
     */
    bool pushSignal(T e);

    /**
     * Retrieves and pops the first element from the queue, if any.
     * 
     * @return the first item, if any, undefined behaviour otherwise.
     */
    T pull();

    /**
     * Retrieves and pops the first element from the queue. If no item is in the
     * queue, the thread is blocked waiting for new items to be inserted.
     * 
     * @return the first item.
     */
    T pullWait();

    /**
     * Returns the number of elements in queue
     */
    size_t size(){return msg_queue.size();}

    /**
     * Returns true if the queue is empty, false otherwise
     */
    size_t empty(){return msg_queue.empty();}
};

// implementation must stay in header since I've used a template

template <typename T, int MAX_SIZE>
MessageQueue<T,MAX_SIZE>::MessageQueue(){
    pthread_mutex_init(&mutex, NULL);
}

template <typename T, int MAX_SIZE>
bool MessageQueue<T,MAX_SIZE>::push(T e){
    bool success;
    pthread_mutex_lock(&mutex);
    if ((success = msg_queue.size() < MAX_SIZE))
        msg_queue.push(e);
    pthread_mutex_unlock(&mutex);
    return success;
}

template <typename T, int MAX_SIZE>
T MessageQueue<T,MAX_SIZE>::pull(){
    T e;
    pthread_mutex_lock(&mutex);
    e = msg_queue.front();
    msg_queue.pop();
    pthread_mutex_unlock(&mutex);
    return e;
}

template <typename T, int MAX_SIZE>
T MessageQueue<T,MAX_SIZE>::pullWait(){
    T e;
    pthread_mutex_lock(&mutex);
    while(msg_queue.empty())
        pthread_cond_wait(&available_messages, &mutex);
    e = msg_queue.front();
    msg_queue.pop();
    pthread_mutex_unlock(&mutex);
    return e;
}

template <typename T, int MAX_SIZE>
bool MessageQueue<T,MAX_SIZE>::pushSignal(T e){
    bool success;
    pthread_mutex_lock(&mutex);
    if ((success = msg_queue.size() < MAX_SIZE)){
        msg_queue.push(e);
        if (msg_queue.size() == 1)
            pthread_cond_signal(&available_messages);
    }
    pthread_mutex_unlock(&mutex);
    return success;

}

#endif // MESSAGE_QUEUE_H
