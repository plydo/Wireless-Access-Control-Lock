// main.h
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
#ifndef _main_h
#define _main_h

#include "c_settings.h" // compile-time settings

#include <Arduino.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#if ENABLE_NTP
#include <NTPClient.h>
#endif

#include "timedout.h" // [ms,sec,min]TimedOut classes, softSeconds(), bootTime
#include "a_settings.h" // stg.* runtime settings

typedef uint32_t uID_t; // type for user id = uid = rfid (someday uint64_t?)

// Global macros

// Logging
void logWrite(int loglevel, const char * const str, ...); // logging.cpp (loge() is used below)
#define logd(...) _logit(1, __VA_ARGS__) // 1 - debug
#define logi(...) _logit(2, __VA_ARGS__) // 2 - info
#define logu(...) _logit(3, __VA_ARGS__) // 3 - user events
#define logw(...) _logit(4, __VA_ARGS__) // 4 - warn
#define loge(...) _logit(5, __VA_ARGS__) // 5 - error
#if 0
#define logf(...) _logit(6, __VA_ARGS__) // 6 - fail  -- unused
#endif
#if 1
#define _logit(level, msg, ...) logWrite(level, msg,##__VA_ARGS__)
#else
#define _logit(level, msg, ...) Serial.printf(msg "\r\n",##__VA_ARGS__) -- old version
#endif

// Global classes, objects, variables

inline constexpr char root_ca[] = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw\n" \
"WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n" \
"RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n" \
"AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP\n" \
"R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx\n" \
"sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm\n" \
"NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg\n" \
"Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG\n" \
"/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB\n" \
"Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA\n" \
"FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw\n" \
"AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw\n" \
"Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB\n" \
"gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W\n" \
"PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl\n" \
"ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz\n" \
"CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm\n" \
"lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4\n" \
"avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2\n" \
"yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O\n" \
"yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids\n" \
"hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+\n" \
"HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv\n" \
"MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX\n" \
"nLRbwHOoq7hHwg==\n" \
"-----END CERTIFICATE-----";

/*
This ACTIVATES a pin.
Pins are treated as active-low if the pin variable is positive and active-high if negative.
That is, positive pin variables are 'normal' and are low when activated and high when deactivated.
If the pin is zero, this does nothing.
*/
inline void activatePin(int pin) {
  if (pin) {
    if (pin > 0)
      digitalWrite(pin, LOW);
    else
      digitalWrite(-pin, HIGH);
  }
}
/*
This DEACTIVATES a pin.
Pins are treated as active-low if the pin variable is positive and active-high if negative.
That is, positive pin variables are 'normal' and are low when activated and high when deactivated.
If the pin is zero, this does nothing.
*/
inline void deactivatePin(int pin) {
  if (pin) {
    if (pin > 0)
      digitalWrite(pin, HIGH);
    else
      digitalWrite(-pin, LOW);
  }
}
/*
This returns 1=HIGH if the pin is ACTIVE, 0=LOW if INACTIVE, and -1=NO_PIN if zero (disabled).
Pins are treated as active-low if positive and active-high if negative. In other words,
positive pin variables are 'normal' and are active-low and normally high until activated.
*/
inline int activatedPin(int pin) {
  if (pin) {
    if (pin > 0)
      return HIGH - digitalRead(pin);
    else
      return digitalRead(-pin);
  } else {
    return -1;
  }
}
#define NO_PIN -1

#define RDM6300_TIMEOUT 500
class rdm6300Class {
public:
  void begin(int8_t rxPin = -1, int8_t txPin = -1);
  uID_t tagID(void);
  uID_t newTagID(void);
  void setTimeout(uint32_t x = RDM6300_TIMEOUT);
};
inline rdm6300Class rdm6300;

class lockClass {
public:
  bool isAccessible() { return isAccessibleVar; }
  void startAccess() { activatePin(stg.outputPin); isAccessibleVar = true;
    lockTimedout.reset(stg.outputMilliseconds ? stg.outputMilliseconds : CURR_CHK_MAX_DELAY); }
  void stopAccess() { deactivatePin(stg.outputPin); isAccessibleVar = false; }
  bool update() {
    if (!isAccessibleVar) return false;
    if (stg.outputMilliseconds) {
      if (lockTimedout) stopAccess();
      return false;
    }
    if (lockTimedout) return false;
    // Got here so it's turned on (accessible) and continuous mode (not pulsed) and not timed-out,
    //   so check current sensor until timer times out
    if (HIGH == activatedPin(stg.currentPin)) { stopAccess(); return true; } // notify user!
    return false;
  }
  minTimedOut autoOffTimedout; // timer for auto-off setting
  time_t ActivatedTime; // time of lock activation in UTC (not local time)
private:
  bool isAccessibleVar; // true if lock is activated
  // For pulsed-output lock mode, this timer is used to generate the pulse.
  // For continuous-output lock mode, this timer is used to check for current after turn-on.
  msTimedOut lockTimedout;
};
inline lockClass lock;

/*
LCD-I2C class customized with added functionality and resetting the display timeout
when updating the display with the print() function
*/
class MyLcd: public LiquidCrystal_I2C {
public:
  using LiquidCrystal_I2C::LiquidCrystal_I2C;
  size_t printf(const char *format, ...);
  size_t print(const String & x) { addon(); return LiquidCrystal_I2C::print(x); }
  size_t print(const char x[]) { addon(); return LiquidCrystal_I2C::print(x); }
  size_t print(char x) { addon(); return LiquidCrystal_I2C::print(x); }
  size_t print(IPAddress x) { addon(); return LiquidCrystal_I2C::print(x); }
  size_t print(unsigned char x, int y = DEC) { addon(); return LiquidCrystal_I2C::print(x, y); }
  size_t print(int x, int y = DEC) { addon(); return LiquidCrystal_I2C::print(x, y); }
  size_t print(unsigned int x, int y = DEC) { addon(); return LiquidCrystal_I2C::print(x, y); }
  size_t print(long x, int y = DEC) { addon(); return LiquidCrystal_I2C::print(x, y); }
  size_t print(unsigned long x, int y = DEC) { addon(); return LiquidCrystal_I2C::print(x, y); }
  // todo? size_t print(const __FlashStringHelper *ifsh) { return print(reinterpret_cast<const char *>(ifsh)); }
  void init() { begin(16, 2); }
  void blinkLight() { isBlinked = true; lcdBlinkTimedOut.reset(250); noBacklight(); } // blink backlight
  void setTimeout() { lcdMsgTimedOut.reset(LCD_TIMER); }
  bool isTimeoutActive() { return lcdMsgTimedOut.isActive(); }
  void saveLine(int line, const char *str) { strlcpy(savedLines[line], str, sizeof savedLines[0]); }
  void printSaved() { clear(); print(savedLines[0]); setCursor(0, 1); print(savedLines[1]); }
  void update() {
    if (lcdMsgTimedOut) { printSaved(); } // update the display after temporarily displaying something
    if (isBlinked && lcdBlinkTimedOut) { isBlinked = false; backlight(); } // turn backlight back on
  }
private:
  void addon(void) { lcdMsgTimedOut.disable(); } // clear any lcd timeout that may be pending
  bool isBlinked; // true if lcd is blinked (backlight is off)
  msTimedOut lcdBlinkTimedOut; // lcd blink (backlight off/on) timer
  secTimedOut lcdMsgTimedOut; // lcd message timer
  char savedLines[2][17]; // used with the lcd msg timer to update the display
};
inline size_t MyLcd::printf(const char *format, ...)
{
    char loc_buf[64];
    va_list arg;
    va_start(arg, format);
    int len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(arg);
    if(len < 0 || len >= (int)sizeof(loc_buf)) return 0;
    for (int x = 0; x < len; x++) LiquidCrystal_I2C::write(loc_buf[x]);
    return len;
}
inline MyLcd lcd((pcf8574Address) 0x27); // 0x27=PCF8574 default (more common), 0x3f=PCF8574A

class uidAdminClass {
#define NUM_ADMIN 5
public:
  void zeroIndex() { index = 0; }
  uID_t next() { return (index < NUM_ADMIN) ? admins[index++] : 0; }
  void load(const char *str);
  secTimedOut startTimedOut;
  secTimedOut adminTimedOut;
private:
  bool add(uID_t x) { if (index < NUM_ADMIN) { admins[index++] = x; return 0; } else { return 1; } }
  int index;
  uID_t admins[NUM_ADMIN];
#undef NUM_ADMIN
};

// This initializes the uidAmin object with the stg.adminIDs string. The string
// may contain several IDs consisting of numeric characters separated by
// by non-numeric characters.
inline void uidAdminClass::load(const char *ptr)
{
  zeroIndex();
  for (; *ptr; ptr++) {
    if (*ptr >= '0' && *ptr <= '9') {
      if (add(strtoul(ptr, nullptr, 10))) {
        loge("Too many admin IDs in configuration file line");
        break;
      }
      while (*ptr >= '0' && *ptr <= '9' && *ptr) ptr++;
    }
  }
}

inline uidAdminClass uidAdmin;

inline uint8_t macAddr[6]; // assigned in setup.cpp
inline bool rebootRequest; // set by webservercode.cpp, acted upon in main.cpp
#if ENABLE_NTP
inline WiFiUDP ntpUDP;
inline NTPClient timeClientNTP(ntpUDP); // used in main.cpp & setup.cpp
#endif

inline minTimedOut webserverTimedout; // initialized in setup.cpp, used by webservercode.cpp,

// Functions in other files

void setupAsyncWebserver(void); // webservercode.cpp
String programInfo(void); // info.cpp, used in webservercode.cpp
time_t localTime(time_t x); // info.cpp
enum formattedTimeMode { // info.cpp
  ftm_yyyymmddhhmmss = 0,
  ftm_yyyymmddhhmm,
  ftm_mmddhhmm,
  ftm_hhmmss,
  ftm_hhmm,
  ftm_yyyymmdd,
};
const char * formattedTime(time_t t, formattedTimeMode mode = ftm_yyyymmddhhmmss); // info.cpp
void serialInfo(); // info.cpp
int subnetIP(); // info.cpp
char * logLatest(void); // logging.cpp
int api_google_sheets(uID_t idTag, unsigned long &idEnable, char idName[]);
int api_bodgery_v0_lookup(uID_t idTag, unsigned long &idEnable, char idName[]);
int api_bodgery_v1_lookup(uID_t idTag, unsigned long &idEnable, char idName[]);
int api_bodgery_v1_add(uID_t idTag);

#endif