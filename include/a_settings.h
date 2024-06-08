// a_settings.h - runtime configuration settings
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
#ifndef _a_settings_h
#define _a_settings_h

// IMPORTANT: When adding/removing settings:
//            1) also update the example config file
//            2) update the default value in settings.cpp if it's not 0/null
#define X_SETTING_LIST /* "X Macro" used for settings, also used in the .cpp */ \
  /*              12345678901324567890 <-- size of settings_defs_t's name field */ \
  X_SETTING(char, deviceName, [32]) \
  X_SETTING(char, deviceGroup, [32]) \
  X_SETTING(char, hostName, [32]) \
  X_SETTING(char, wifiSSID, [32]) \
  X_SETTING(char, wifiPassword, [32]) \
  X_SETTING(char, webserverUsername, [32]) \
  X_SETTING(char, webserverPassword, [32]) \
  X_SETTING(char, backendUsername, [32]) \
  X_SETTING(char, backendSecret, [132]) /* bearer token is 128 bytes */ \
  X_SETTING(char, backendURL, [96]) \
  X_SETTING(char, adminIDs, [64]) \
  X_SETTING(int, backendType, ;) /* 0=none, ... */ \
  X_SETTING(int, logLevelFile, ;) \
  X_SETTING(int, logLevelSerial, ;) \
  X_SETTING(int, logFileMax, ;) \
  X_SETTING(int, webserverMinutes, ;) /* 0=never, default=99,999,999 */ \
  X_SETTING(int, readerType, ;) /* 0=none, 1=MFRC522, 2=PN532, 3=rdm6300, 4=Wiegand */ \
  X_SETTING(int, currentPin, ;) /* 0=none, negative=inactive-low/active-high */ \
  X_SETTING(int, voltagePin, ;) /* 0=none, negative=inactive-low/active-high */ \
  X_SETTING(int, beeperPin, ;) /* 0=none, negative=inactive-low/active-high */ \
  X_SETTING(int, outputPin, ;) /* 0=none, negative=inactive-low/active-high */ \
  X_SETTING(int, rx2Pin, ;) /* rdm6300, etc. */ \
  X_SETTING(int, tx2Pin, ;) /* rdm6300, etc. */ \
  X_SETTING(int, outputMilliseconds, ;) /* 0=toggle (continuous) */ \
  X_SETTING(int, autoOffMinutes, ;) /* auto-turn off lock (if machine is off), 0=never */ \
// end of X_SETTINGs

class programSettings {
public:
#define X_SETTING(type, name, length) type name length;
  X_SETTING_LIST 
#undef X_SETTING
  void loadSettings(); // loads from file
};

inline programSettings stg;

#endif