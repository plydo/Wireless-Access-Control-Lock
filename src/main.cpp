// main.cpp - main program, contains the Arduino loop()
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
#if ENABLE_NTP
#include <TimeLib.h> // for setTime
#endif

/*
This sets up the LCD object so the LCD displays a usage summary after the LCD
message timeout finishes. This also disables the auto-off timer.
*/
void machineOffSetup()
{
  #define BUFFER_SIZE 17
  char buffer[BUFFER_SIZE];

  // 0123456789012345
  // mm/dd,hh:mm ZZZm
  strlcpy(buffer, formattedTime(localTime(lock.ActivatedTime), ftm_mmddhhmm), BUFFER_SIZE);
  #pragma GCC diagnostic ignored "-Wformat-truncation"
  snprintf(buffer + 11, sizeof buffer - 11, " %3lim",
    (now() - lock.ActivatedTime + 30) / 60);
  #pragma GCC diagnostic pop
  lcd.saveLine(1, buffer);
  #undef BUFFER_SIZE
  lock.autoOffTimedout.disable();
}

/*
This handles auto-turnoff of the machine/lock if enabled in the settings.
This also handles the realtime updating of the time on the LCD while the
machine/lock is enabled/activated.
*/
void machineTimeoutUpdate()
{
  static time_t autoOffStart; // auto-off start time

  if (!lock.isAccessible()) // machine is already off
    return;
  // Update the machine runtime on the LCD while it's turned-on/accessible
  if (!lcd.isTimeoutActive()) { // if not displaying a temporary message...
    #define BUFFER_SIZE 17
    char buffer[17];

    int sec = now() - lock.ActivatedTime;
    lcd.setCursor(0, 1);
    // 0123456789012345
    // ON mmm:ss MMM:SS  where MMM:SS is the time the machine is enabled but unpowered
    snprintf(buffer, sizeof buffer, "ON%4i:%02i", sec / 60, sec % 60);
    if (lock.autoOffTimedout.isActive()) {
      sec = now() - autoOffStart;
      sec = 60 * stg.autoOffMinutes - sec;
      if (sec < 0) sec = 0;
      #pragma GCC diagnostic ignored "-Wformat-truncation"
      snprintf(buffer + 9, sizeof buffer - 9, "%4i:%02i", sec / 60, sec % 60);
      #pragma GCC diagnostic pop
    } else {
      strlcat(buffer, "       ", BUFFER_SIZE);
    }
    lcd.print(buffer);
    #undef BUFFER_SIZE
  }
  if (stg.autoOffMinutes == 0) // auto-off is disabled
    return;
  if (activatedPin(stg.currentPin)) { // machine is drawing current or pin isn't used
    if (lock.autoOffTimedout.isActive()) // must've powered back on, so stop auto-off timeout
     lock.autoOffTimedout.disable();
    return;
  }
  // At this point we know that:
  //   1) machine is unlocked, 2) auto-off setting is enabled, 3) machine isn't drawing current
  if (lock.autoOffTimedout.isDisabled()) { // start auto-off timer if it's not started
    lock.autoOffTimedout.reset(stg.autoOffMinutes);
    autoOffStart = now();
    return;
  }
  if (lock.autoOffTimedout) { // perform auto-off
    lock.stopAccess();
    machineOffSetup();
    lcd.setCursor(0, 1);
    //          01234567898012345
    lcd.printf("%i min auto-OFF", stg.autoOffMinutes);
    lcd.setTimeout();
    logu("Auto-off (%im), used %li minutes",
      stg.autoOffMinutes, (now() - lock.ActivatedTime + 30) / 60);
    return;
  }
}

/*
This looks up the ID (1st arg) using the backend. It returns the enable in the
2nd arg and the name in the 3rd arg. If not found, it returns ID_NOT_FOUND
2nd arg and the name found in the 3rd arg. If not found, it returns ID_NOT_FOUND
in the 2nd arg. Also, the fcn return value is nonzero if an error occurred.

This may call functions that use internet API calls to perform the lookup.
These can take awhile, especially when using the https protocol.
This can block the main thread for 0.2 to 2 seconds.
*/
int lookupID(uID_t uid, unsigned long &idEnable, char idName[])
{
  int error = 0;

  switch (stg.backendType) {
    case 0: // standalone -- just testing for now
      idEnable = (uid == 2455917) ? 1 : 0; // for testing
      strlcpy(idName, "John Doe", ID_NAME_MAX);
      error = 0;
      break;
    case 1: // Bodgery V0
      error = api_bodgery_v0_lookup(uid, idEnable, idName);
      break;
    case 2: // Bodgery V1
      error = api_bodgery_v1_lookup(uid, idEnable, idName);
      break;
    default:
      error = 999;
      break;
  }
  return error;
}

/*
This checks user input for an ID. If an ID is input, it looks up the ID and
allows/denies access based on the lookup info.
*/
void processID(void)
{
  char idName[ID_NAME_MAX];
  unsigned long idEnable;

  uID_t uid = rdm6300.newTagID();
  if (uid == 0) return; // no RFID ready
#if 0 // background blink -- bad, the lookup can block turning the backlight back on
  lcd.blinkLight(); // turn backlight off/on to indicate RFID was read
#else // foreground blink -- looks good
  lcd.noBacklight(); // turn backlight off/on to indicate RFID was read
  delay(250);
  lcd.backlight();
#endif
  lcd.clear();
  lcd.print("WAIT...");
  // This may take time, like 0.1-1.9 seconds, blocking loop() and software timers (ie lcd blink)
  int error = lookupID(uid, idEnable, idName);
  lcd.init(); // re-init/clear lcd (just in case it got stuck)
  do { // <-- not a do-loop, just used for "break;" statements
    if (uid == 0) break;

    if (error) {
      //         0123456789012345
      lcd.print("Access Rejected");
      lcd.setCursor(0, 1);
      lcd.printf("%i Lookup Fail", error);
      logu("ID '%010u' lookup failed with error %i", uid, error);
      break;
    }

    if (idEnable == ID_NOT_FOUND) {
      //         0123456789012345
      lcd.print("Access Rejected");
      lcd.setCursor(0, 1);
      lcd.print("No Record for ID");
      logu("Rejected ID '%010u' for no record", uid);
      break;
    }

    #define ADMIN_TIMEOUT 50
    uidAdmin.zeroIndex();
    if (uidAdmin.adminTimedOut.isActive()) // keep active while scanning key fobs
      uidAdmin.adminTimedOut.reset(ADMIN_TIMEOUT);
    while (1) { // uidAdmin search loop
      uID_t adminID = uidAdmin.next();
      if (adminID == 0) break; // no matching admin IDs
      if (adminID == uid) {
        if (uidAdmin.adminTimedOut.isActive()) { // 3rd admin scan, stop admin mode
          uidAdmin.adminTimedOut.disable();
          lcd.print(idName);
          lcd.setCursor(0, 1);
          //         0123456789012345
          lcd.print("Adding Stopped");
          logd("Admin mode stopped by key fob");
          uid = 0;
        } if (uidAdmin.startTimedOut) { // 1st admin scan, wait for 2nd admin scan
          uidAdmin.startTimedOut.reset(10);
        } else { // 2nd admin scan, start admin mode
          uidAdmin.adminTimedOut.reset(ADMIN_TIMEOUT);
          lcd.print(idName);
          lcd.setCursor(0, 1);
          //         0123456789012345
          lcd.print("Scan fobs to add");
          logi("Admin mode started by '%s'", idName);
          uid = 0;
          if (lock.isAccessible()) {
            lock.stopAccess();
            machineOffSetup();
            logu("Auto-off (%im), used %li minutes",
              stg.autoOffMinutes, (now() - lock.ActivatedTime + 30) / 60);
          }
        }
        break;
      } // if Admin ID matches
    } // uidAdmin search loop
    #undef ADMIN_TIMEOUT
    if (uid == 0) break;

    lcd.print(idName);
    lcd.setCursor(0, 1);

    if (uidAdmin.adminTimedOut.isActive()) {
      error = (stg.backendType == 2) ? api_bodgery_v1_add(uid) : 100;
      if (error) {
        lcd.printf("Add Error %i", error);
        logw("Adding '%s' failed with error %i", idName, error);
        break;
      }
      lcd.print("Added");
      logi("Added '%s' to active list", idName);
      break;
    }

    if (!idEnable) {
      lcd.print("Access Rejected");
      logu("Rejected '%s', access denied", idName);
      break;
    }
    if (!lock.isAccessible()) { // machine is disabled/off so turn it on
      if (HIGH == activatedPin(stg.voltagePin)) {
        //         0123456789012345
        lcd.print("Turn off. Rescan");
        logu("Turn off & re-scan to turn on, '%s'", idName);
        break;
      } else {
        lcd.saveLine(0, idName);
        lcd.saveLine(1, ""); // machineTimeoutUpdate() updates this line
        lcd.print("Enabled - ON");
        lock.startAccess();
        lock.ActivatedTime = now();
        logu("Accepted '%s', turned on", idName);
        break;
      }
    }
    if (lock.isAccessible()) { // machine already enabled/on so turn it off
      if (HIGH == activatedPin(stg.currentPin)) {
        //         0123456789012345
        lcd.print("Turn off. Rescan");
        logu("Turn off & re-scan before turning off, '%s'", idName);
        break;
      } else {
        lock.stopAccess();
        machineOffSetup();
        //         0123456789012345
        lcd.print("Machine is OFF");
        logu("Accepted '%s', turned off, used %li minutes",
          idName, (now() - lock.ActivatedTime + 30) / 60);
        break;
      }
    }
  } while (0);
  lcd.setTimeout(); // clear above LCD message after a little while
  return;
}

/* This is for timed jobs of about 5m or more. This gets run at least every minute. */
void minuteJobs()
{
#if ENABLE_NTP
  static minTimedOut NTPTimedout; // periodically get accurate time from a server
  if (NTPTimedout) {
    NTPTimedout.reset(59); // minutes
    if (WiFi.status() == WL_CONNECTED) {
      if (timeClientNTP.forceUpdate()) { // uses the Internet, has a one second timeout
        setTime(timeClientNTP.getEpochTime());
        if (softSeconds() / 3600 / 24 > 365 * 10 /*years*/) { // then bootTime==0, so fix it
          bootTime = now();
          logw("Setting boot time now because NTP failed during initial boot");
        }
      }
    }
  }
#endif
  // ADD MORE MINUTE-TIMED JOBS HERE
}

// This is for timed jobs of about 10s-5m. This gets run approximately once every second.
void secondsLongJobs()
{
  static unsigned sixteenSeconds;
  if ((sixteenSeconds++ & 0x0f) == 0) minuteJobs();

  static secTimedOut WiFiDisconnectTimedout; // reconnect WiFi Station if it disconnected
  static secTimedOut WifIDisMsgTimedout; // for disconnect log message
  if (WiFiDisconnectTimedout) {
    WiFiDisconnectTimedout.reset(60);
    if (WiFi.status() != WL_CONNECTED) {
      // Note: this could probably be replaced with
      //   WiFi.setAutoReconnect(true);
      //   WiFi.persistent(true);
      // right after wifi.begin in setup.cpp but then we couldn't log the disconnect issue
      WiFiDisconnectTimedout.reset(60);
      WiFi.reconnect(); // this doesn't do anything when WiFi mode is set to WIFI_AP
      if (WifIDisMsgTimedout) {
        WifIDisMsgTimedout.reset(3600);
        logw("WiFi Disconnected - reconnect attempted");
      }
    }
  }
  if (uidAdmin.adminTimedOut) {
    lcd.clear();
    //         0123456789012345
    lcd.print("No longer adding");
    lcd.setCursor(0, 1);
    //         0123456789012345
    lcd.print("fobs by scanning");
    lcd.setTimeout(); // clear above message after a little while
    logd("Admin mode stopped by timeout");
  }
  if (uidAdmin.adminTimedOut.isActive()) {
    lcd.setCursor(15, 0); // admin-mode indicator -- also disables LCD timeout
    lcd.print("*");
  }
  machineTimeoutUpdate();
  // ADD MORE SECOND-TIMED JOBS HERE
}

/*
 This is for timed jobs of about 10s or less. This gets run frequently -- every loop iteration.
 IMPORTANT: An expired timer will retrigger in ~25 days because of 32-bit signed int comparison,
            so this is most useful for repetitive events. The second and minute timers are
            more useful for single-time events.
*/
void millisecondJobs()
{
  static msTimedOut oneSecondTimedout;
  if (oneSecondTimedout) {
    oneSecondTimedout.reset(1000);
    secondsLongJobs();
  }
  static msTimedOut ledTimedout;
  if (ledTimedout) { // fast-blink=OK, slow-blink=no-WiFi, stuck-on/off=froze-up
    ledTimedout.reset((WiFi.status() == WL_CONNECTED) ? 17 : 1500);
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ? LOW : HIGH);
  }
  if (lock.update()) { // then the lock object is telling us AC-current was detected on power-on
    lcd.setCursor(0, 1);
    //         0123456789012345
    lcd.print("Turn Off. Rescan");
  }
  lcd.update();
  // ADD MORE MILLISECOND-TIMED JOBS HERE
}

// This is used with the serial interface to test the scanner's working distance
static void testScanner(void)
{
  static uID_t idLast = 0;
  static msTimedOut TestTimedout;
  uID_t uid;

  if (TestTimedout) {
    TestTimedout.reset(200);
    uid = rdm6300.tagID();
    if (idLast == uid) {
      if (uid) Serial.print('.'); // indicate tag is still present
    } else if (uid) {
        Serial.printf("\r\n%u", uid); // new tag
    }
    idLast = uid;
  }
}

// This handles user input from the serial port for use in debugging
// It returns true if in debugging mode and false for normal mode
static bool processSerialDebug(void)
{
  static bool testModeForScanner = false;

  if (Serial.available()) {
    int x = Serial.read();
    while (Serial.read() >= 0) /*NULL*/; // flush the serial input
    if (x == 't') {
      if (testModeForScanner) {
        testModeForScanner = false;
        rdm6300.setTimeout();
        Serial.print("RFID test mode stopped.\r\n");
      } else {
        testModeForScanner = true;
        rdm6300.setTimeout(50);
        Serial.print("RFID test mode started. Press 't' again to exit test mode.\r\n");
      }
    }
    if (x == ' ') serialInfo();
  }
  if (testModeForScanner) testScanner();
  return testModeForScanner;
}

void loop()
{
  millisecondJobs(); // timed (intermittent) background jobs

  if(rebootRequest) { // note: the source of a reboot request should log the reason
    delay(100);
    ESP.restart();
  }

  if (!processSerialDebug()) {
    processID(); // Main job of the program -- process user ID's
  }
} // loop end