/**
 * @file logging.h
 * @author Riccardo Mancini
 * 
 * @brief Logging macro.
 * 
 * This file contains a macro for logging in different levels.
 *
 * There are 5 levels of logging:
 *  - fatal (LOG_FATAL) 
 *  - error (LOG_ERROR)
 *  - warning (LOG_WARN)
 *  - information (LOG_INFO)
 *  - debug (LOG_DEBUG)
 * 
 * The first three will be outputted to stderr, the latter two to stdout.
 * 
 * You can define the LOG_LEVEL macro to one of the available levels for 
 * hiding some of the logging messages (default: debug).
 * 
 * TODO: logging may overlap in a concurrent environment
 * 
 * Adapted from https://stackoverflow.com/a/328660
 */

#ifndef LOGGING
#define LOGGING


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#define TZ_OFFSET (2)


#define LOG_FATAL    (1)
#define LOG_ERR      (2)
#define LOG_WARN     (3)
#define LOG_INFO     (4)
#define LOG_DEBUG    (5)

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_DEBUG
#endif

inline const char* logColor(int level){
  switch(level){
    case LOG_FATAL:
      return "\033[31m";
    case LOG_ERR:
      return "\033[91m";
    case LOG_WARN:
      return "\033[33m";
    case LOG_INFO:
      return "\033[32m";
    case LOG_DEBUG:
      return "\033[94m";
    default:
      return "\033[0m";
  }
} 

inline void printtime(FILE* dbgstream){
  timeval tv;
  int ret = gettimeofday(&tv, NULL);
  if(ret == -1){
    return;
  }
  
  unsigned int hour = tv.tv_sec % (24*60*60) / (60*60);
  hour += TZ_OFFSET;
  hour %= 24;
  unsigned int min = tv.tv_sec % (60*60) / 60;
  unsigned int sec = tv.tv_sec % (60);
  unsigned int msec = tv.tv_usec / 1000;
  fprintf(dbgstream, "[%02u:%02u:%02u.%03u]", hour, min, sec, msec );
}

#define LOG(level, ...) do {  \
                          if (level <= LOG_LEVEL) { \
                            FILE *dbgstream; \
                            char where[50]; \
                            switch(level){ \
                              case LOG_FATAL: \
                                dbgstream = stderr; \
                                fprintf(dbgstream, "%s[FATAL]", logColor(LOG_FATAL)); \
                                break; \
                              case LOG_ERR: \
                                dbgstream = stderr; \
                                fprintf(dbgstream, "%s[ERROR]", logColor(LOG_ERR)); \
                                break; \
                              case LOG_WARN: \
                                dbgstream = stderr; \
                                fprintf(dbgstream, "%s[WARN ]", logColor(LOG_WARN)); \
                                break; \
                              case LOG_INFO: \
                                dbgstream = stdout; \
                                fprintf(dbgstream, "%s[INFO ]", logColor(LOG_INFO)); \
                                break; \
                              case LOG_DEBUG: \
                                dbgstream = stdout; \
                                fprintf(dbgstream, "%s[DEBUG]", logColor(LOG_DEBUG)); \
                                break; \
                            } \
                            fprintf(dbgstream, "[%-5d]", (int) getpid()); \
                            printtime(dbgstream); \
                            snprintf(where, 50, "[%s:%d]", __FILE__, __LINE__); \
                            fprintf(dbgstream, " %-25s ", where); \
                            fprintf(dbgstream, __VA_ARGS__); \
                            fprintf(dbgstream, "\033[0m\n"); \
                            fflush(dbgstream); \
                          } \
                        } while (0)

#define LOG_PERROR(level, ...) LOG(level, __VA_ARGS__, strerror(errno))

#endif