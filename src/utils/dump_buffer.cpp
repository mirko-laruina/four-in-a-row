/**
 * @file dump_buffer.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of dump_buffer.h.
 * 
 * @see dump_buffer.h
 */

#include "utils/dump_buffer.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void dump_buffer_hex(char* buffer, int len){
  char *str, tmp[4];
  int i;

  str = (char*) malloc(len*3+1);

  str[0] = '\0';
  for (i=0; i<len; i++){
    sprintf(tmp, "%02x ", (unsigned char) buffer[i]);
    strcat(str, tmp);
  }

  LOG(LOG_DEBUG, "%s", str);
  free(str);
}
