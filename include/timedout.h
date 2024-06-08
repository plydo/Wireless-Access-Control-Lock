// timedout.h - software timing support
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
#include <Arduino.h>
#include <TimeLib.h>

/*
Seconds at boot, used to compute uptime, etc.
If there is no NTP server or other time source, this is zero.
If there is an NTP server or other time source, this is
seconds since the 1970 epoch which will fail in the year 2038.
*/
inline time_t bootTime;
#define TIME_T_MAX INT_MAX

inline unsigned softSeconds() { return now() - bootTime; } // seconds since boot

/*
Use this for repetitive events or use with a separate boolean for one-time events. Examples:
  static msTimedOut blinkTimedOut; // zero or no value will time out right away
  if (blinkTimedOut) {
    blinkTimedOut.reset(500); // milliseconds
    ToggleLED();
  }

  static bool eventEnable; // Event setup: eventEnable=true; eventTimedOut.reset(500);
  static msTimedOut eventTimedOut;
  if (eventEnable && eventTimedOut) {
    eventEnable = false;
    eventAction();
  }
*/
class msTimedOut {
  public:
    void reset(long x) { timeoutMillis = millis() + x; };
    msTimedOut(long x = 0) { reset(x); };
    operator bool() { // conversion operator -- object returns true if timed out
      long timeLeft = (int) (timeoutMillis - millis());
      return (timeLeft > 0) ? false : true; // signed comparison prevents wraparound issue
    };
  private:
    unsigned long timeoutMillis; // a future millis() value
};

/*
This has the same usage as msTimedOut except that timedOut only happens once and then
is disabled so that this can be used for one-time events without an additional variable
*/
class secTimedOut {
  public:
    void disable() { timeoutSec = TIME_T_MAX; };
    bool isActive() { return (timeoutSec != TIME_T_MAX) && (timeoutSec > softSeconds()); }
    bool isDisabled() { return (timeoutSec == TIME_T_MAX); }
    void reset(long x) { timeoutSec = softSeconds() + x; };
    secTimedOut(long x = 0) { reset(x); }
    operator bool() { // conversion operator -- object returns true if timed out and enabled
      if (timeoutSec > softSeconds()) {
        return false; // not timed out OR disabled
      } else {
        disable();
        return true; // was timed out AND was enabled
      }
    };
  protected:
    time_t timeoutSec; // a future softSeconds() value
};

/*
This has the same usage as secTimedOut except the timeout value is in minutes instead of seconds
*/
class minTimedOut : public secTimedOut {
  public:
    void reset(long x) { secTimedOut::timeoutSec = softSeconds() + x * 60; };
};