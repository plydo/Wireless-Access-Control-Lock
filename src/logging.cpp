// logging.cpp - error logging, especially logging locally to a file
/*
Copyright 2024 Mark Pickhard
Copyright rights associated with this file are nonexclusively transferred to The Bodgery Inc,
  a 501c(3) nonprofit entity.
This file is part of WACL. WACL is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.
WACL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
  Public License for more details.
You should have received a copy of the GNU General Public License along with WACL. If not, see
  <https://www.gnu.org/licenses/>.
*/
#include "main.h"
#include <LittleFS.h>

// This logs data to the serial port and the filesystem if the loglevel is >=
// the setting. This limits the size of the log on the filesystem.
void logWrite(int loglevel, const char * const str, ...)
{
  #define BUFFER_SIZE 256 // pretty big because json response debug line is long
  char buffer[BUFFER_SIZE];
  va_list arg;

  // printf to buffer with a timestamp prefix and a CRLF=\r\n suffix
  strlcpy(buffer, formattedTime(localTime(now())), BUFFER_SIZE);
  // 01234567890123456789
  // yyyy-mm-dd,hh:mm:ss
  buffer[19] = ' ';
  va_start(arg, str);
  (void) vsnprintf(buffer + 20, BUFFER_SIZE - 20 - 2, str, arg);
  va_end(arg);
  strlcat(buffer, "\r\n", BUFFER_SIZE);

  // log to serial port
  if (!(loglevel < abs(stg.logLevelSerial) || (loglevel == 3 && stg.logLevelSerial < 0))) {
    Serial.print(buffer);
  }

  // log to file
  if (!(loglevel < abs(stg.logLevelFile) || (loglevel == 3 && stg.logLevelFile < 0))) {
    if (stg.logFileMax == 0) return;
    File logfile = LittleFS.open(LOG_FILE_CURRENT, "a"); // creates file if it doesn't exist
    if (!logfile) return;
    if (!logfile.print(buffer)) { logfile.close(); return; }
    int logsize = logfile.size();
    logfile.close();
    if (logsize < stg.logFileMax) return;
    if (LittleFS.exists(LOG_FILE_OLDER)) LittleFS.remove(LOG_FILE_OLDER);
    LittleFS.rename(LOG_FILE_CURRENT, LOG_FILE_OLDER);
  }
  #undef BUFFER_SIZE
}

#define NUM_LINES 5
#define BYTES_PER_LINE 70 // average, estimate
#define BUFFER_SIZE (BYTES_PER_LINE * NUM_LINES)

// This is a helper fcn for logLatest(). It returns the number of bytes read.
static int logPartial(uint8_t * buff, int bsize, const char * filename)
{
  File logfile = LittleFS.open(filename, "r");
  if (!logfile) return 0;
  int logsize = logfile.size();
  int logseek = logsize - bsize;
  if (logseek < 0) logseek = 0;
  if (!logfile.seek(logseek, SeekSet)) { logfile.close(); return 0; }
  int bytesread = logfile.read(buff, bsize);
  logfile.close();
  return bytesread;
}

// This returns the last few lines logged. If there's not enough lines in the
// most recent log file, it looks in the older log file for more lines.
char * logLatest(void)
{
  static uint8_t buffer[BUFFER_SIZE];

  File logfile = LittleFS.open(LOG_FILE_CURRENT, "r");
  int logsize = (logfile) ? logfile.size() : 0;
  logfile.close();
  int bytesread1 = 0;
  if (logsize < BUFFER_SIZE - 1) {
    bytesread1 = logPartial(buffer, BUFFER_SIZE - 1 - logsize, LOG_FILE_OLDER);
  }
  int bytesread2 = 0;
  if (logsize != 0) {
    bytesread2 = logPartial(buffer + bytesread1, min(logsize, BUFFER_SIZE - 1),
      LOG_FILE_CURRENT);
  }
  buffer[bytesread1 + bytesread2]  = '\0';
  if (bytesread1 + bytesread2 == BUFFER_SIZE - 1) {
    // skip past any partial line at the beginning of the buffer
    uint8_t *ptr;
    for (ptr = buffer; ; ptr++) {
      if (*ptr == '\n') return (char *) (ptr + 1);
      if (*ptr == '\0') return (char *) ptr;
    }
  }
  return (char *) buffer;
}

#undef NUM_LINES
#undef BYTES_PER_LINE
#undef BUFFER_SIZE