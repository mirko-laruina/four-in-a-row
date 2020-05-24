  /**
 * @file messages.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of messages.h
 * 
 * @see messages.h
 */

#include <cstdlib>
#include <cstring>

#include "network/messages.h"
#include "network/inet_utils.h"

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
        case REGISTER:
            m = new RegisterMessage;
            break;
        case CHALLENGE:
            m = new ChallengeMessage;
            break;
        case GAME_END:
            m = new GameEndMessage;
            break;
        case USERS_LIST:
            m = new UsersListMessage;
            break;
        case USERS_LIST_REQ:
            m = new UsersListRequestMessage;
            break;
        case CHALLENGE_FWD:
            m = new ChallengeForwardMessage;
            break;
        case CHALLENGE_RESP:
            m = new ChallengeResponseMessage;
            break;
        case GAME_START:
            m = new GameStartMessage;
            break;
        case GAME_CANCEL:
            m = new GameCancelMessage;
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

msglen_t RegisterMessage::write(char *buffer){
    buffer[0] = (char) REGISTER;
    strncpy(&buffer[1], username.c_str(), MAX_USERNAME_LENGTH);
    return 1+username.size();
}

msglen_t RegisterMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = string(&buffer[1], min(MAX_USERNAME_LENGTH, len-1));
    return 0;
}

msglen_t ChallengeMessage::write(char *buffer){
    buffer[0] = (char) CHALLENGE;
    strncpy(&buffer[1], username.c_str(), MAX_USERNAME_LENGTH);
    return 1+username.size();    
}
msglen_t ChallengeMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = string(&buffer[1], min(MAX_USERNAME_LENGTH, len-1));
    return 0;
}

msglen_t GameEndMessage::write(char *buffer){
    buffer[0] = (char) GAME_END;
    return 1;
}
msglen_t GameEndMessage::read(char *buffer, msglen_t len){
    return 0;
}

msglen_t UsersListMessage::write(char *buffer){
    buffer[0] = (char) USERS_LIST;
    strncpy(&buffer[1], usernames.c_str(), MAX_USERNAME_LENGTH*MAX_USERS);
    return 1+usernames.size();  
}
msglen_t UsersListMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    usernames = string(&buffer[1], min(MAX_USERNAME_LENGTH*MAX_USERS, len-1));
    return 0;
}

msglen_t UsersListRequestMessage::write(char *buffer){
    buffer[0] = (char) USERS_LIST_REQ;
    *((uint32_t*)&buffer[1]) = htonl(offset);
    return 1 + sizeof(offset);
}
msglen_t UsersListRequestMessage::read(char *buffer, msglen_t len){
    if (len < 1 + sizeof(offset))
        return 1;
    offset = ntohl(*((uint32_t*) &buffer[1]));
    return 0;
}

msglen_t ChallengeForwardMessage::write(char *buffer){
    buffer[0] = (char) CHALLENGE_FWD;
    strncpy(&buffer[1], username.c_str(), MAX_USERNAME_LENGTH);
    return 1+username.size();    
}
msglen_t ChallengeForwardMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = string(&buffer[1], min(MAX_USERNAME_LENGTH, len-1));
    return 0;
}

msglen_t ChallengeResponseMessage::write(char *buffer){
    buffer[0] = (char) CHALLENGE_RESP;
    buffer[1] = (char) response;
    strncpy(&buffer[2], username.c_str(), MAX_USERNAME_LENGTH);
    return 2+username.size();   
}
msglen_t ChallengeResponseMessage::read(char *buffer, msglen_t len){
    if (len < 1 + 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    response = (bool) buffer[1];
    username = string(&buffer[2], min(MAX_USERNAME_LENGTH, len-2));
    return 0;
}

msglen_t GameCancelMessage::write(char *buffer){
    buffer[0] = (char) GAME_CANCEL;
    strncpy(&buffer[1], username.c_str(), MAX_USERNAME_LENGTH);
    return 1+username.size();    
}
msglen_t GameCancelMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = string(&buffer[1], min(MAX_USERNAME_LENGTH, len-1));
    return 0;
}

msglen_t GameStartMessage::write(char *buffer){
    buffer[0] = (char) GAME_START;
    sockaddr_in_to_buffer(addr, &buffer[1]);
    strncpy(&buffer[1+SERIALIZED_SOCKADDR_IN_LEN],
         username.c_str(), 
         MAX_USERNAME_LENGTH);
    return username.size()+sizeof(addr)+1;
}
msglen_t GameStartMessage::read(char *buffer, msglen_t len){
    if (len < 1 + SERIALIZED_SOCKADDR_IN_LEN + MIN_USERNAME_LENGTH+1)
        return 1;
    addr = buffer_to_sockaddr_in(&buffer[1]);
    username = string(&buffer[1+SERIALIZED_SOCKADDR_IN_LEN], 
        min(MAX_USERNAME_LENGTH, len-1-SERIALIZED_SOCKADDR_IN_LEN));
    return 0;
}
