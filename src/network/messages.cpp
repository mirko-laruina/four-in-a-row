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
#include "utils/buffer_io.h"

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
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) START_GAME_PEER)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t StartGameMessage::read(char *buffer, msglen_t len){
    return 0;
}

msglen_t MoveMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) MOVE)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, col)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t MoveMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readUInt8((uint8_t*)&col, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t RegisterMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) REGISTER)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, username)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t RegisterMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readUsername(&username, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t ChallengeMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CHALLENGE)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, username)) < 0)
        return 0;
    i += ret;

    return i;
}
msglen_t ChallengeMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readUsername(&username, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t GameEndMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) GAME_END)) < 0)
        return 0;
    
    i += ret;

    return i;
}
msglen_t GameEndMessage::read(char *buffer, msglen_t len){
    return 0;
}

msglen_t UsersListMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) USERS_LIST)) < 0)
        return 0;
    i += ret;

    size_t strsize = min(usernames.size(), 
                         (size_t) ((MAX_USERNAME_LENGTH+1)*MAX_USERS));
    size_t padded_size = (strsize+MAX_USERNAME_LENGTH)/(MAX_USERNAME_LENGTH+1)*(MAX_USERNAME_LENGTH+1);
    if ((int)padded_size > MAX_MSG_SIZE-i)
        return 0;
    strncpy(&buffer[i], usernames.c_str(), strsize);
    memset(&buffer[1+strsize], 0, padded_size-strsize+1);
    i += padded_size+1;

    return i;
}

msglen_t UsersListMessage::read(char *buffer, msglen_t len){
    int maxsize = min((MAX_USERNAME_LENGTH+1)*MAX_USERS, len-1);
    if (maxsize <= 0){
        return 1;
    }

    usernames = string(&buffer[1], maxsize);
    return 0;
}

msglen_t UsersListRequestMessage::write(char *buffer){
        int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) USERS_LIST_REQ)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUInt32(&buffer[i], MAX_MSG_SIZE-i, offset)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t UsersListRequestMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readUInt32(&offset, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t ChallengeForwardMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CHALLENGE_FWD)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, username)) < 0)
        return 0;
    i += ret;

    return i;
}
msglen_t ChallengeForwardMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readUsername(&username, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t ChallengeResponseMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CHALLENGE_RESP)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBool(&buffer[i], MAX_MSG_SIZE-i, response)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUInt16(&buffer[i], MAX_MSG_SIZE-i, listen_port)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, username)) < 0)
        return 0;
    i += ret;

    return i;
}
msglen_t ChallengeResponseMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readBool(&response, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUInt16(&listen_port, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUsername(&username, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t GameCancelMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) GAME_CANCEL)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, username)) < 0)
        return 0;
    i += ret;

    return i;
}
msglen_t GameCancelMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readUsername(&username, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t GameStartMessage::write(char *buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) GAME_START)) < 0)
        return 0;
    i += ret;

    if ((ret = writeSockAddrIn(&buffer[i], MAX_MSG_SIZE-i, addr)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, username)) < 0)
        return 0;
    i += ret;

    if ((ret = cert2buf(cert, &buffer[i], MAX_MSG_SIZE - i)) < 0)
        return 0;    
    i += ret;

    return i;
}

msglen_t GameStartMessage::read(char *buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readSockAddrIn(&addr, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUsername(&username, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;
    
    if ((ret = buf2cert(&buffer[i], len - i, &cert)) < 0)
        return 1;
    i += ret;
    
    return 0;
}

msglen_t SecureMessage::write(char* buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) SECURE_MESSAGE)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBuf(&buffer[i], MAX_MSG_SIZE-i, ct, ct_size)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBuf(&buffer[i], MAX_MSG_SIZE-i, tag, TAG_SIZE)) < 0)
        return 0;
    i += ret;

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
    int ret;

    if ((ret = readBuf(ct, ct_size, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;
    
    if ((ret = readBuf(tag, TAG_SIZE, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;
    
    return 0;
}

msglen_t ClientHelloMessage::write(char* buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CLIENT_HELLO)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBuf(&buffer[i], MAX_MSG_SIZE-i, (char*) &nonce, sizeof(nonce))) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, my_id)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, other_id)) < 0)
        return 0;
    i += ret;
    
    if ((ret = pkey2buf(eph_key, &buffer[i], MAX_MSG_SIZE-i)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t ClientHelloMessage::read(char* buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readBuf((char*)&nonce, sizeof(nonce), &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUsername(&my_id, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUsername(&other_id, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;
    
    if((ret = buf2pkey(&buffer[i], len-i, &eph_key)) < 0)
        return 1;
    i += ret;

    return 0;
}

ServerHelloMessage::~ServerHelloMessage(){
    if (ds != NULL){
        free(ds);
    }
}

msglen_t ServerHelloMessage::write(char* buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) SERVER_HELLO)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBuf(&buffer[i], MAX_MSG_SIZE-i, (char*) &nonce, sizeof(nonce))) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, my_id)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUsername(&buffer[i], MAX_MSG_SIZE-i, other_id)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUInt32(&buffer[i], MAX_MSG_SIZE-i, ds_size)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBuf(&buffer[i], MAX_MSG_SIZE-i, ds, ds_size)) < 0)
        return 0;
    i += ret;

    if ((ret = pkey2buf(eph_key, &buffer[i], MAX_MSG_SIZE-i)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t ServerHelloMessage::read(char* buffer, msglen_t len){
    int i = 1;
    int ret;

    if ((ret = readBuf((char*)&nonce, sizeof(nonce), &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUsername(&my_id, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUsername(&other_id, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    if ((ret = readUInt32(&ds_size, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    ds = (char*) malloc(ds_size);
    if (!ds){
        LOG_PERROR(LOG_ERR, "Malloc failed: %s");
        return 1;
    }

    if ((ret = readBuf(ds, ds_size, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;
    
    if((ret = buf2pkey(&buffer[i], len-i, &eph_key)) < 0)
        return 1;
    i += ret;

    return 0;
}

ClientVerifyMessage::~ClientVerifyMessage(){
    if (ds != NULL){
        free(ds);
    }
}

msglen_t ClientVerifyMessage::write(char* buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CLIENT_VERIFY)) < 0)
        return 0;
    i += ret;

    if ((ret = writeUInt32(&buffer[i], MAX_MSG_SIZE-i, ds_size)) < 0)
        return 0;
    i += ret;

    if ((ret = writeBuf(&buffer[i], MAX_MSG_SIZE-i, ds, ds_size)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t ClientVerifyMessage::read(char* buffer, msglen_t len){
   int i = 1;
    int ret;

    if ((ret = readUInt32(&ds_size, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    ds = (char*) malloc(ds_size);
    if (!ds){
        LOG_PERROR(LOG_ERR, "Malloc failed: %s");
        return 1;
    }

    if ((ret = readBuf(ds, ds_size, &buffer[i], len-i)) < 0)
        return 1;
    i += ret;

    return 0;
}

msglen_t CertificateRequestMessage::write(char* buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CERT_REQ)) < 0)
        return 0;
    
    i += ret;

    return i;
}

msglen_t CertificateRequestMessage::read(char* buffer, msglen_t len){
    return 0;
}

msglen_t CertificateMessage::write(char* buffer){
    int i = 0;
    int ret;

    if ((ret = writeUInt8(&buffer[i], MAX_MSG_SIZE-i, (char) CERTIFICATE)) < 0)
        return 0;
    
    i += ret;

    if ((ret = cert2buf(cert, &buffer[i], MAX_MSG_SIZE-1)) < 0)
        return 0;
    i += ret;

    return i;
}

msglen_t CertificateMessage::read(char* buffer, msglen_t len){
    int ret = buf2cert(&buffer[1], len-1, &cert);
    return ret > 0 ? 0 : 1;
}
