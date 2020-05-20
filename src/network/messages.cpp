  /**
 * @file messages.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of messages.h
 * 
 * @see messages.h
 */

#include <cstdlib>

#include "network/messages.h"

Message* readMessage(char *buffer, msglen_t len){
    Message *m;
    int ret;

    switch(buffer[0]){
        case START_GAME:
            m = new StartGameMessage;
            break;
        case MOVE:
            m = new MoveMessage;
            break;
        default:
            m = NULL;
            LOG(LOG_ERR, "Unrecognized message type %d", buffer[0]);
            return NULL;
    };

    ret = m->read(buffer, len);

    if (ret != 0){
        LOG(LOG_ERR, "Error reading message of type %d: %d", buffer[0], ret);
        return NULL;
    } else{
        return m;
    }
}


msglen_t StartGameMessage::write(char *buffer){
    buffer[0] = (char) START_GAME;
    return 1;
}

msglen_t StartGameMessage::read(char *buffer, msglen_t len){
    return 0;
}

msglen_t MoveMessage::write(char *buffer){
    buffer[0] = (char) MOVE;
    buffer[1] = col;
    return 2;
}

msglen_t MoveMessage::read(char *buffer, msglen_t len){
    if (len < 2)
        return 1;

    col = buffer[1];
    return 0;
}