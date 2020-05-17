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
#include "utils/dump_buffer.h"

Message* readMessage(char *buffer, int len){
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
            dump_buffer_hex(buffer, len);
            return NULL;
    };

    ret = m->read(buffer, len);

    if (ret != 0){
        LOG(LOG_ERR, "Error reading message of type %d: %d", buffer[0], ret);
        dump_buffer_hex(buffer, len);
        return NULL;
    } else{
        return m;
    }
}


int StartGameMessage::write(char *buffer){
    buffer[0] = (char) START_GAME;
    return 1;
}

int StartGameMessage::read(char *buffer, int len){
    return 0;
}

int MoveMessage::write(char *buffer){
    buffer[0] = (char) MOVE;
    buffer[1] = col;
    return 2;
}

int MoveMessage::read(char *buffer, int len){
    if (len < 2)
        return 1;

    col = buffer[1];
    return 0;
}