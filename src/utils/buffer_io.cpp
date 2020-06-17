/**
 * @file dump_buffer.h
 * @author Riccardo Mancini
 * 
 * @brief Utility functions for writing and reading data from a buffer
 * 
 * @date 2020-06-16
 */

#ifndef BUFFER_IO_H
#define BUFFER_IO_H

#include <cstring>
#include <stdint.h>
#include "utils/buffer_io.h"
#include "network/inet_utils.h"

using namespace std;

int readBool(bool *val, char* buf, size_t buf_size){
    if (buf_size < sizeof(bool))
        return -1;
    
    *val = (bool) buf[0];
    return sizeof(bool);
}

int writeBool(char* buf, size_t buf_size, bool val){
    if (buf_size < sizeof(bool))
        return -1;
    
    buf[0] = (char) val;
    return sizeof(bool);
}

int readUInt32(uint32_t *val, char* buf, size_t buf_size){
    if (buf_size < sizeof(uint32_t))
        return -1;

    *val = ntohl(*((uint32_t*) buf));
    return sizeof(uint32_t);
}

int writeUInt32(char* buf, size_t buf_size, uint32_t val){
    if (buf_size < sizeof(uint32_t))
        return -1;

    *((uint32_t*)buf) = htonl(val);
    return sizeof(uint32_t);
}

int readUInt16(uint16_t *val, char* buf, size_t buf_size){
    if (buf_size < sizeof(uint16_t))
        return -1;

    *val = ntohs(*((uint16_t*) buf));
    return sizeof(uint16_t);
}

int writeUInt16(char* buf, size_t buf_size, uint16_t val){
    if (buf_size < sizeof(uint16_t))
        return -1;

    *((uint16_t*)buf) = htons(val);
    return sizeof(uint16_t);
}

int readUInt8(uint8_t *val, char* buf, size_t buf_size){
    if (buf_size < sizeof(uint8_t))
        return -1;
    
    *val = (uint8_t) buf[0];
    return sizeof(uint8_t);
}

int writeUInt8(char* buf, size_t buf_size, uint8_t val){
    if (buf_size < sizeof(uint8_t))
        return -1;
    
    buf[0] = val;
    return sizeof(uint8_t);
}

int readBuf(char *val, size_t len, char* buf, size_t buf_size){
    if (buf_size < len)
        return -1;
    
    memcpy(val, buf, len);
    return len;
}

int writeBuf(char* buf, size_t buf_size, char* val, size_t len){
    if (buf_size < len)
        return -1;
    
    memcpy(buf, val, len);
    return len;
}

#endif // BUFFER_IO_H