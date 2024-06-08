// c_settings.h - compile-time settings
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
#ifndef _c_settings_h
#define _c_settings_h

#define PROJECT_SHORT "WACL"
#define PROJECT_LONG  "Wireless Access Control Lock"
#define VERSION "1.00"

// These may be changed as needed
#define DEF_DEVICE_NAME "dummy"
#define DEF_WIFI_SSID ""
#define DEF_WIFI_PASSWORD ""
#define DEF_WEB_USERNAME "admin"
#define DEF_WEB_PASSWORD "admin"
#define DEF_LOG_LEVEL 3
#define DEF_WEBSERVER_MINUTES 99999999 // enable webserver more-or-less forever
#define LCD_TIMER 20 // number of seconds to display temporary LCD messages
// Wait this long between turning on the machine and checking to see if there's
//   electrical current because the machine was already on
#define CURR_CHK_MAX_DELAY 1500 // long enough for sensor response but too short for user

#define ENABLE_DNS 0 // enables mDNS server
#define ENABLE_NTP 1 // enables using NTP server to set time
#define ENABLE_BACKEND_BODGERY_V0 1 // include code for this backend
#define ENABLE_BACKEND_BODGERY_V1 1 // include code for this backend
#define ENABLE_BACKEND_GOOGLE_SHEETS 1 // include code for this backend
#define ENABLE_BACKEND_BUDIBASE 1 // include code for this backend

// It is recommended that these not be changed unless you really like being different
#define LED_BUILTIN 2
#define ID_NAME_MAX 50 // for fixed-length buffers
#define ID_NOT_FOUND -1 // id not found during lookup
inline constexpr char SETTINGS_FILE[] = "/config.txt"; // LittleFS requires the leading '/'
inline constexpr char LOG_FILE_CURRENT[] = "/log-current.txt";
inline constexpr char LOG_FILE_OLDER[] = "/log-previous.txt";

#endif