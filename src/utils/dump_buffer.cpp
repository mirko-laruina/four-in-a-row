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
#include <cctype>

#define ROW 32


void dump_buffer_hex(char* buffer, int len, int log_level){
  char *str, tmp3[4], tmp1[2];
  int i, j;
  int n_rows = (len+ROW-1)/ROW;

  str = (char*) malloc(n_rows*(3*ROW + 4 + ROW + 1)+1);

  const char* col_sep = "    ";
  const char* row_sep = "\n";

  str[0] = '\0';
  for (i=0; i<n_rows; i ++){
    for (j=0; j<ROW; j++){
      int idx = i*ROW+j;
      if (idx < len){
        sprintf(tmp3, "%02x ", (unsigned char) buffer[idx]);
      } else {
        sprintf(tmp3, "   ");
      }
      strcat(str, tmp3);
    }

    strcat(str, col_sep);

    for (j=0; j<ROW; j++){
      int idx = i*ROW+j;
      if (idx >= len)
        break;

      if (isprint(buffer[idx])){
        sprintf(tmp1, "%c", buffer[idx]);
      } else{
        sprintf(tmp1, ".");
      }
      strcat(str, tmp1);
    }

    strcat(str, row_sep);
  }
  LOG(log_level, "Buffer dump");
  if (log_level >= LOG_LEVEL)
    printf("%s%s\033[0m\n", logColor(log_level), str);
  free(str);
}
