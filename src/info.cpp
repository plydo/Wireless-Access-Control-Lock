// info.cpp - code that provides status information
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

#if ENABLE_NTP
#include <TimeZone.h>

static size_t heapInitialFree; // for programInfo report

/* This calculates the local time from UTC (GMT) time.  */
time_t localTime(time_t x)
{
  static TimeChangeRule myDST = {"D", Second, Sun, Mar, 2, -300 /*min*/}; // summer timezone
  static TimeChangeRule mySTD = {"S", First,  Sun, Nov, 2, -360}; // winter/std timezone
  static Timezone myTZ(myDST, mySTD); // time offsets from UTC to local time
  return (x < 3600*24*365*20) ? x : myTZ.toLocal(x); // don't adjust if 1970-1990 (unset time)
}
#else
time_t localTime(time_t x) { return x; }
#endif

/* This returns uptime character string "xd xxh xxm" */
char * uptime()
{
  #define BUFFER_SIZE 15
  // 12345678901234
  // xxxxd xxh xxm0 or alternatively this could be something like: 1123+13:45:23
  static char buffer[BUFFER_SIZE];
  unsigned minutes, hours, days;

  minutes = softSeconds() / 60;
  days = minutes / (60 * 24);
  minutes = minutes % (60 * 24);
  hours = minutes / 60;
  minutes = minutes % 60;
  #pragma GCC diagnostic ignored "-Wformat-truncation"
  snprintf(buffer, BUFFER_SIZE, "%ud %uh %um", days, hours, minutes);
  #pragma GCC diagnostic pop
  return buffer;
  #undef BUFFER_SIZE
}

/* This returns the subnet number, typically 24 (sometimes written like 192.168.0.1/24) */
int subnetIP()
{
  int sum;
  uint32_t mask =
    (WiFi.status() == WL_CONNECTED) ? WiFi.subnetMask() : WiFi.softAPSubnetMask();
  for (sum = 0; mask; mask >>= 1)
    if (mask & 1) sum++;
  return sum;
}

/*
This returns the date & time (1st argument) formatted as a c-string formatted as
"yyyy-mm-dd,hh:mm:ss" if the 'mode' is missing, or per the ftm_X 'mode' argument
name used (e.g. "hh:mm" for ftm_hhmm). See main.h for ftm_... definitions.
*/
const char * formattedTime(time_t t, formattedTimeMode mode)
{
  #define BUFFER_SIZE 20
  // 01234567890123456789
  // yyyy-mm-dd,hh:mm:ss0 -- this is the longest version
  static char buffer[BUFFER_SIZE];
  #pragma GCC diagnostic ignored "-Wformat-truncation" // for snprintf
  snprintf(buffer, BUFFER_SIZE, "%4i-%02i-%02i,%02i:%02i:%02i",
    year(t), month(t), day(t), hour(t), minute(t), second(t));
  #pragma GCC diagnostic pop
  switch (mode) {
    case ftm_yyyymmddhhmmss:
      return buffer;
    case ftm_yyyymmddhhmm:
      buffer[16] = '\0';
      return buffer;
    case ftm_mmddhhmm:
      buffer[16] = '\0';
      return buffer + 5;
    case ftm_hhmmss:
      return buffer + 11;
    case ftm_hhmm:
      buffer[16] = '\0';
      return buffer + 11;
    case ftm_yyyymmdd:
      buffer[10] = '\0';
      return buffer;
    default:
      return "FTERR"; // programming errorbuffer;
  }
  #undef BUFFER_SIZE
}

/* This returns a string of text info for display in the webserver */
String programInfo()
{
  #define stringf(...) { snprintf(buffer, BUFFER_SIZE, __VA_ARGS__); ret += buffer; }
  #define kb(x) ((x)/1024)
  #define BUFFER_SIZE 100
  char buffer[BUFFER_SIZE];
  String ret;
  int inPinNum;
  const char * inPinVlt;
  const char * inPinTxt;

  stringf("Device Name: %s\n" "Hostname:    %s\n", stg.deviceName, stg.hostName);
  stringf("Software:    " PROJECT_SHORT " - " PROJECT_LONG ", V" VERSION " (built " __DATE__ ")\n");
  stringf("Hardware:    CPU %s-%u @ %u MHz, Flash %u kb @ %u MHz\n",
    ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(),
    kb(ESP.getFlashChipSize()), ESP.getFlashChipSpeed() / 1000000
  );
  stringf("Filesystem:  %u kb Total, %u kb Free, %u kb Used\n", kb(LittleFS.totalBytes()),
    kb(LittleFS.totalBytes() - LittleFS.usedBytes()), kb(LittleFS.usedBytes()));
  stringf("Program:     %u kb Total (2x%u), %u kb (%u%%) Used\n",
    2 * kb(ESP.getFreeSketchSpace()), kb(ESP.getFreeSketchSpace()),
    kb(ESP.getSketchSize()), 100 * ESP.getSketchSize() / ESP.getFreeSketchSpace()
  );
  stringf("Heap (kb):   %u initial, %u (%u) current, %u lowest, %u largest block\n",
    kb(heapInitialFree),
    kb(heap_caps_get_free_size(MALLOC_CAP_DEFAULT)), kb(ESP.getFreeHeap()),
    kb(heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT)),
    kb(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT))
  );
  stringf("WiFi:        RSSI %i, IP %s/%i, MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
    WiFi.RSSI(), (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str()
                                                : WiFi.softAPIP().toString().c_str(), subnetIP(),
    macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]
  );
  { // current sensor pin
    inPinNum = activatedPin(stg.currentPin);
    inPinVlt = "--";
    inPinTxt = inPinNum ? (inPinNum == HIGH ?  "Active" : "Disabled") : "Inactive";
    if ((stg.currentPin > 0 && inPinNum == HIGH)
      || (stg.currentPin < 0 && inPinNum == LOW)) inPinVlt = "Gnd";
    if ((stg.currentPin > 0 && inPinNum == LOW)
      || (stg.currentPin < 0 && inPinNum == HIGH)) inPinVlt = "+V";
    stringf("Current Pin: Pin %i, %s=%i=%s (current/voltage sensor input)\n",
            stg.currentPin, inPinVlt, inPinNum, inPinTxt);
  }
  if (stg.voltagePin != stg.currentPin) { // voltage sensor pin
    inPinNum = activatedPin(stg.voltagePin);
    inPinVlt = "--";
    inPinTxt = inPinNum ? (inPinNum == HIGH ?  "Active" : "Disabled") : "Inactive";
    if ((stg.voltagePin > 0 && inPinNum == HIGH)
      || (stg.voltagePin < 0 && inPinNum == LOW)) inPinVlt = "Gnd";
    if ((stg.voltagePin > 0 && inPinNum == LOW)
      || (stg.voltagePin < 0 && inPinNum == HIGH)) inPinVlt = "+V";
    stringf("Voltage Pin: Pin %i, %s=%i=%s (current/voltage sensor input)\n",
            stg.voltagePin, inPinVlt, inPinNum, inPinTxt);
  }
  const char * outPinTxt = stg.outputPin ? (stg.outputPin > 0 ?  "Active Low" : "Active High") :
    "Disabled";
  stringf("Output Pin:  Pin %i, %s", stg.outputPin, outPinTxt);
  if (stg.outputPin) {
    if (stg.outputMilliseconds) {
      stringf(", Pulsed for %i ms", stg.outputMilliseconds);
    } else {
      stringf(", Continuous, Toggled");
    }
  }
  stringf(" (lock/relay output)\n");
  stringf("Date/Time:   %s\n", formattedTime(localTime(now())));
  stringf("Boot Time:   %s   Uptime: %s\n", formattedTime(localTime(bootTime)), uptime());
  stringf("\nRecent Log Entries:\n");
  ret += logLatest();
  return ret;
  #undef BUFFER_SIZE
  #undef kb
  #undef stringf
}

void serialInfo()
{
  Serial.printf(
    "Hardware: %s-%u, %u MHz; Program: %u/%u used\r\n"
    "  Min Heap: %u; IP: %s/%i; RSSI: %i; Uptime: %s\r\n"
    , ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz()
    , ESP.getSketchSize(), ESP.getSketchSize() + ESP.getFreeSketchSpace()
    , heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT)
    , (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str()
                                     : WiFi.softAPIP().toString().c_str()
    , subnetIP() , WiFi.RSSI(), uptime()
  );
  time_t t = now();
  Serial.printf("  Date/Time: UTC:%s", formattedTime(t));
  Serial.printf(", Local:%s\r\n", formattedTime(localTime(t)));
  if (0 == heapInitialFree) // for programInfo report
    heapInitialFree = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}
