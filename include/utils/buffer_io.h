/**
 * @file dump_buffer.h
 * @author Riccardo Mancini
 * 
 * @brief Utility functions for writing and reading data from a buffer
 * 
 * These functions are buffer-overflow-safe, i.e. they check the remaining 
 * buffer length before writing/reading. The return is -1 in case of errors, 
 * the written/read size otherwise.
 * 
 * @date 2020-06-16
 */

#ifndef BUFFER_IO_H
#define BUFFER_IO_H

#include <cstdlib>
#include <stdint.h>

using namespace std;

/**
 * Reads a boolean.
 * 
 * @param val the dest value
 * @param buf the source buffer
 * @param buf_size the size of the buffer
 * @returns -1 in case of errors
 * @returns 1 number of read bytes
 */
int readBool(bool *val, char* buf, size_t buf_size);

/**
 * Writes a boolean.
 * 
 * @param buf the dest buffer
 * @param buf_size the size of the buffer
 * @param val the source value
 * @returns -1 in case of errors
 * @returns 1 number of read bytes
 */
int writeBool(char* buf, size_t buf_size, bool val);

/**
 * Reads a uint32_t.
 * 
 * @param val the dest value
 * @param buf the source buffer
 * @param buf_size the size of the buffer
 * @returns -1 in case of errors
 * @returns 4 number of read bytes
 */
int readUInt32(uint32_t *val, char* buf, size_t buf_size);

/**
 * Writes a uint32_t.
 * 
 * @param buf the dest buffer
 * @param buf_size the size of the buffer
 * @param val the source value
 * @returns -1 in case of errors
 * @returns 4 number of read bytes
 */
int writeUInt32(char* buf, size_t buf_size, uint32_t val);

/**
 * Reads a uint16_t.
 * 
 * @param val the dest value
 * @param buf the source buffer
 * @param buf_size the size of the buffer
 * @returns -1 in case of errors
 * @returns 2 number of read bytes
 */
int readUInt16(uint16_t *val, char* buf, size_t buf_size);

/**
 * Writes a uint16_t.
 * 
 * @param buf the dest buffer
 * @param buf_size the size of the buffer
 * @param val the source value
 * @returns -1 in case of errors
 * @returns 2 number of read bytes
 */
int writeUInt16(char* buf, size_t buf_size, uint16_t val);

/**
 * Reads a uint8_t.
 * 
 * @param val the dest value
 * @param buf the source buffer
 * @param buf_size the size of the buffer
 * @returns -1 in case of errors
 * @returns 1 number of read bytes
 */
int readUInt8(uint8_t *val, char* buf, size_t buf_size);

/**
 * Writes a uint8_t.
 * 
 * @param buf the dest buffer
 * @param buf_size the size of the buffer
 * @param val the source value
 * @returns -1 in case of errors
 * @returns 1 number of read bytes
 */
int writeUInt8(char* buf, size_t buf_size, uint8_t val);


/**
 * Reads a char array.
 * 
 * @param val the dest buffer
 * @param len number of bytes to read
 * @param buf the source buffer
 * @param buf_size the size of the buffer
 * @returns -1 in case of errors
 * @returns len number of read bytes
 */
int readBuf(char *val, size_t len, char* buf, size_t buf_size);

/**
 * Writes a char array.
 * 
 * @param buf the dest buffer
 * @param buf_size the size of the buffer
 * @param val the source buffer
 * @param len number of bytes to write
 * @returns -1 in case of errors
 * @returns len number of read bytes
 */
int writeBuf(char* buf, size_t buf_size, char* val, size_t len);

#endif // BUFFER_IO_H