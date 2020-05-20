/**
 * @file dump_buffer.h
 * @author Riccardo Mancini
 * 
 * @brief Utility function for dumping a buffer as hex string.
 * 
 * @date 2020-05-17
 */

#ifndef DUMP_BUFFER_H
#define DUMP_BUFFER_H


/**
 * Prints content of buffer to stdout, showing it as hex values.
 * 
 * It uses the logging infrastructure to print.
 * 
 * @param buffer    pointer to the buffer to be printed
 * @param len       the length (in bytes) of the buffer
 */
void dump_buffer_hex(char* buffer, int len, int log_level);

#if LOG_LEVEL == LOG_DEBUG
#define DUMP_BUFFER_HEX_DEBUG(buffer, len) dump_buffer_hex((buffer), (len), LOG_DEBUG)
#else
#define DUMP_BUFFER_HEX_DEBUG(buffer, len)
#endif


#endif // DUMP_BUFFER_H
