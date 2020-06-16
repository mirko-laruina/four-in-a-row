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
#include <cmath>

#include "network/messages.h"
#include "network/inet_utils.h"

#include "security/crypto_utils.h"

Message* readMessage(char *buffer, msglen_t len){
    Message *m;
    int ret;

    switch(buffer[0]){
        case SECURE_MESSAGE:
            m = new SecureMessage;
            break;
        case CLIENT_HELLO:
            m = new ClientHelloMessage;
            break;
        case SERVER_HELLO:
            m = new ServerHelloMessage;
            break;
        case CLIENT_VERIFY:
            m = new ClientVerifyMessage;
            break;
        case START_GAME_PEER:
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
        case CERT_REQ:
            m = new CertificateRequestMessage;
            break;
        case CERTIFICATE:
            m = new CertificateMessage;
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
    buffer[0] = (char) START_GAME_PEER;
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
    size_t strsize = writeUsername(username, &buffer[1]);
    return 1+strsize;
}

msglen_t RegisterMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = readUsername(&buffer[1], len-1);
    return 0;
}

msglen_t ChallengeMessage::write(char *buffer){
    buffer[0] = (char) CHALLENGE;
    size_t strsize = writeUsername(username, &buffer[1]);
    return 1+strsize;  
}
msglen_t ChallengeMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = readUsername(&buffer[1], len-1);
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
    size_t strsize = min(usernames.size(), 
                         (size_t) ((MAX_USERNAME_LENGTH+1)*MAX_USERS));
    size_t padded_size = (strsize+MAX_USERNAME_LENGTH)/(MAX_USERNAME_LENGTH+1)*(MAX_USERNAME_LENGTH+1);
    strncpy(&buffer[1], usernames.c_str(), strsize);
    memset(&buffer[1+strsize], 0, padded_size-strsize+1);
    return 2+padded_size;  
}
msglen_t UsersListMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    usernames = string(&buffer[1], min((MAX_USERNAME_LENGTH+1)*MAX_USERS, len-1));
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
    size_t strsize = writeUsername(username, &buffer[1]);
    return 1+strsize;
}
msglen_t ChallengeForwardMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = readUsername(&buffer[1], len-1);
    return 0;
}

msglen_t ChallengeResponseMessage::write(char *buffer){
    buffer[0] = (char) CHALLENGE_RESP;
    buffer[1] = (char) response;
    uint16_t nport = htons(listen_port);
    memcpy(&buffer[2], &nport, sizeof(nport));
    size_t strsize = writeUsername(username, &buffer[4]);
    return 4+strsize;
}
msglen_t ChallengeResponseMessage::read(char *buffer, msglen_t len){
    if (len < 1 + 1 + 2 + MIN_USERNAME_LENGTH+1)
        return 1;
    response = (bool) buffer[1];
    uint16_t nlisten_port;
    memcpy(&nlisten_port, &buffer[2], sizeof(nlisten_port));
    listen_port = ntohs(nlisten_port);
    username = readUsername(&buffer[4], len-4);
    return 0;
}

msglen_t GameCancelMessage::write(char *buffer){
    buffer[0] = (char) GAME_CANCEL;
    size_t strsize = writeUsername(username, &buffer[1]);
    return 1+strsize; 
}
msglen_t GameCancelMessage::read(char *buffer, msglen_t len){
    if (len < 1 + MIN_USERNAME_LENGTH+1)
        return 1;
    username = readUsername(&buffer[1], len-1);
    return 0;
}

msglen_t GameStartMessage::write(char *buffer){
    int i = 0;
    buffer[i] = (char) GAME_START;
    i++;

    sockaddr_in_to_buffer(addr, &buffer[i]);
    i += SERIALIZED_SOCKADDR_IN_LEN;
    size_t strsize = writeUsername(username, &buffer[i]);
    i += strsize;
    size_t available_len = MAX_MSG_SIZE - i;
    size_t certsize = cert2buf(&cert, &buffer[i], available_len);    
    i += certsize;
    return i;
}

msglen_t GameStartMessage::read(char *buffer, msglen_t len){
    if (len < 1 + SERIALIZED_SOCKADDR_IN_LEN + MIN_USERNAME_LENGTH+1)
        return 1;
    int offset = 1;
    addr = buffer_to_sockaddr_in(&buffer[1]);
    offset += SERIALIZED_SOCKADDR_IN_LEN;
    username = readUsername(&buffer[offset], len-offset);
    offset += MAX_USERNAME_LENGTH + 1;
    int ret = buf2cert(&buffer[offset], len - offset, &cert);
    if (ret < 0){
        return 1;
    }
    return 0;
}

msglen_t SecureMessage::write(char* buffer){
    int i = 0;

    buffer[i] = (char) SECURE_MESSAGE;
    i++;

    memcpy(&buffer[i], ct, ct_size);
    i += ct_size;

    memcpy(&buffer[i], tag, TAG_SIZE);
    i += TAG_SIZE;

    return i;
}

msglen_t SecureMessage::read(char* buffer, msglen_t len){
    ct_size = len-1-TAG_SIZE;
    ct = (char*) malloc(ct_size);
    tag = (char*) malloc(TAG_SIZE);
    if(!ct || !tag){
        LOG(LOG_WARN, "Malloc failed for message of length %d", len);
        return -1;
    }

    int i = 1;

    memcpy(ct, &buffer[i], ct_size);
    i += ct_size;
    memcpy(tag, &buffer[i], TAG_SIZE);
    return 0;
}

msglen_t ClientHelloMessage::write(char* buffer){
    int i = 0;
    buffer[i] = (MessageType) CLIENT_HELLO;
    i++;
    memcpy(&buffer[i], &nonce, sizeof(nonce));
    i += sizeof(nonce);
    size_t strsize;
    strsize = writeUsername(my_id, &buffer[i]);
    i+= strsize;
    strsize = writeUsername(other_id, &buffer[i]);
    i+= strsize;
    int ret = pkey2buf(&eph_key, &buffer[i], MAX_MSG_SIZE-i);
    if (ret > 0){
        i += ret;
        return i;
    } else {
        return -1;
    }
}

msglen_t ClientHelloMessage::read(char* buffer, msglen_t len){
    int i = 1;
    memcpy(&nonce, &buffer[i], sizeof(nonce));
    i += sizeof(nonce);
    my_id = readUsername(&buffer[i], len-i);
    i += MAX_USERNAME_LENGTH + 1;
    other_id = readUsername(&buffer[i], len-i);
    i += MAX_USERNAME_LENGTH + 1;
    int ret = buf2pkey(&buffer[i], len-i, &eph_key);
    return ret > 0 ? 0 : 1;
}

ServerHelloMessage::~ServerHelloMessage(){
    if (ds != NULL){
        free(ds);
    }
}

msglen_t ServerHelloMessage::write(char* buffer){
    int i = 0;
    buffer[i] = (MessageType) SERVER_HELLO;
    i++;
    memcpy(&buffer[i], &nonce, sizeof(nonce));
    i += sizeof(nonce);
    size_t strsize;
    strsize = writeUsername(my_id, &buffer[i]);
    i+= strsize;
    strsize = writeUsername(other_id, &buffer[i]);
    i+= strsize;
    memcpy(&buffer[i], ds, DS_SIZE);
    i += DS_SIZE;
    int ret = pkey2buf(&eph_key, &buffer[i], MAX_MSG_SIZE-i);
    if (ret > 0){
        i += ret;
        return i;
    } else {
        return -1;
    }
}

msglen_t ServerHelloMessage::read(char* buffer, msglen_t len){
    ds = (char*) malloc(DS_SIZE);
    if (!ds){
        LOG_PERROR(LOG_ERR, "Malloc failed: %s");
        return 1;
    }
    int i = 1;
    memcpy(&nonce, &buffer[i], sizeof(nonce));
    i += sizeof(nonce);
    my_id = readUsername(&buffer[i], len-i);
    i += MAX_USERNAME_LENGTH + 1;
    other_id = readUsername(&buffer[i], len-i);
    i += MAX_USERNAME_LENGTH + 1;
    memcpy(ds, &buffer[i], DS_SIZE);
    i += DS_SIZE;
    int ret = buf2pkey(&buffer[i], len-i, &eph_key);
    return ret > 0 ? 0 : 1;
}

ClientVerifyMessage::~ClientVerifyMessage(){
    if (ds != NULL){
        free(ds);
    }
}

msglen_t ClientVerifyMessage::write(char* buffer){
    int i = 0;
    buffer[i] = (MessageType) CLIENT_VERIFY;
    i++;
    memcpy(&buffer[i], ds, DS_SIZE);
    i += DS_SIZE;
    return i;
}

msglen_t ClientVerifyMessage::read(char* buffer, msglen_t len){
    ds = (char*) malloc(DS_SIZE);
    if (!ds){
        LOG_PERROR(LOG_ERR, "Malloc failed: %s");
        return 1;
    }
    int i = 1;
    memcpy(ds, &buffer[i], DS_SIZE);
    i += DS_SIZE;
    return 0;
}

msglen_t CertificateRequestMessage::write(char* buffer){
    buffer[0] = (MessageType) CERT_REQ;
    return 1;
}

msglen_t CertificateRequestMessage::read(char* buffer, msglen_t len){
    return 0;
}

msglen_t CertificateMessage::write(char* buffer){
    int i = 0;
    buffer[i] = (MessageType) CERTIFICATE;
    i++;
    int ret = cert2buf(&cert, buffer, MAX_MSG_SIZE-1);
    if (ret <= 0)
        return -1;
    i += ret;
    return i;
}

msglen_t CertificateMessage::read(char* buffer, msglen_t len){
    int ret = buf2cert(buffer, len, &cert);
    return ret > 0 ? 0 : 1;
}
