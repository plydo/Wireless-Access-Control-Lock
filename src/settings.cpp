// a_settings.cpp - runtime configuration settings
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

/*
This initializes the project's settings. The settings are stored as text similar to
a ".ini" or ".yaml" file. See the example config file for details on the file format.
Examples (the first line is the canonical/preferred format):
  log-level = 1 # <-preferred, comments may be on the same line or separate lines
  Log_level : 1 ; another comment
  LogLevel  = 1
  loglevel:1
  setting-str = Single or double quotes for strings are also OK
*/

#include "main.h"
#include "ini_by_benhoyt.h"
#include <LittleFS.h>

#define initString(storage, constant) strlcpy(storage, constant, sizeof constant)

// This sets any not-zero/not-null default values
static void initSettings()
{
  initString(stg.deviceName, DEF_DEVICE_NAME);
  initString(stg.wifiSSID, DEF_WIFI_SSID);
  initString(stg.wifiPassword, DEF_WIFI_PASSWORD);
  initString(stg.webserverUsername, DEF_WEB_USERNAME);
  initString(stg.webserverPassword, DEF_WEB_PASSWORD);
  stg.logLevelFile = DEF_LOG_LEVEL;
  stg.logLevelSerial = DEF_LOG_LEVEL;
  stg.webserverMinutes = DEF_WEBSERVER_MINUTES;
  stg.rx2Pin = -1; // hardware default
  stg.tx2Pin = -1; // hardware default
  sprintf(stg.hostName, PROJECT_SHORT "-%02x%02x%02x", macAddr[3], macAddr[4], macAddr[5]);
}

// Definitions for each setting, used to transfer the settings stored in the
// file to the variables in the program
typedef struct {
  char type[6]; // either "char" or "int" or "float"
  char name[20]; // e.g. "wifiPassword" or "webserverMinutes"
  char size[6]; // e.g. "[32]" for char or ";" for int
  void * ptr; // pointer to the variable in the program
} settings_defs_t;
static const settings_defs_t settingsDefs[] = {
  #define X_SETTING(type, name, length) { #type, #name, #length, &stg.name},
  X_SETTING_LIST 
  #undef X_SETTING
  { "", "", "", NULL } // indicates end
};

// This is a helper for ini_handler_fcn.
// This compares setting names and returns true if they match. Both strings are
// assumed to be null-terminated. The file string is assumed to be lowercase and
// not containing '-' or '_'. The program string is case insensitive.
static bool nameMatch(const char *pgmStr, const char *fileStr)
{
  bool eos1 = false, eos2 = false; // end of string
  for (;;) {
    if (*pgmStr == '_') { pgmStr++; continue; } // skip '_' in settingsDefs[]
    if (*pgmStr == '\0') eos1 = true;
    if (*fileStr == '\0') eos2 = true;
    if (eos1 && eos2) return true; // identical contents and lengths
    if (tolower(*pgmStr) != *fileStr) return false; // different contents
    if (eos1 || eos2) return false; // different lengths
    pgmStr++; fileStr++;
  }
}

// This is a helper for reading config file -- it handles each config entry
static int ini_handler_fcn(void* user /*unused*/, const char* setting_section /*unused*/,
  const char* setting_name, const char* setting_value, int lineno)
{
  char settingName[100];

  {
    const char *src = setting_name;
    char *dest = settingName; // setting_name with '-' & '_' removed

    while (1) { // make setting name from file all lowercase and remove '-' & '_'
      *dest = *src++;
      if (*dest == '\0') break;
      *dest = tolower(*dest);
      if (*dest != '-' && *dest != '_') dest++;
    }
  }
  for (int x = 0; settingsDefs[x].name[0] != '\0'; x++) {
    if (nameMatch(settingsDefs[x].name, settingName)) { // settings name match
      if (settingsDefs[x].type[0] == 'i') { // set the variable to the value
        int z = strtol(setting_value, nullptr, 10);
        * (int *) (settingsDefs[x].ptr) = z;
        logd("Setting %s=%s='%i'", settingsDefs[x].name, setting_name, z);
      } else if (settingsDefs[x].type[0] == 'f') {
        float z = atof(setting_value);
        * (float *) (settingsDefs[x].ptr) = z;
        logd("Setting %s=%s=%f", settingsDefs[x].name, setting_name, z);
      } else if (settingsDefs[x].type[0] == 'c') {
        bool quoted = false;
        int srcLen = strlen(setting_value);
        if ((setting_value[0] == '"' || setting_value[0] == '\'') &&
            setting_value[srcLen - 1] == setting_value[0]
        ) quoted = true;
        int destLen = strtol(&settingsDefs[x].size[1], nullptr, 10);
        if (destLen <=2)
          loge("Programming error: config string length invalid");
        strlcpy((char *) (settingsDefs[x].ptr), &setting_value[quoted ? 1 : 0],
          quoted ? min(destLen, srcLen - 1) : destLen);
        logd("Setting %s[%i]=%s='%s'",
          settingsDefs[x].name, destLen, setting_name, (char *) settingsDefs[x].ptr);
      } else {
        loge("Programming error: config type not handled");
      }
      return 1;
    } // if
  } // for
  loge("Unknown setting '%s' with value '%s' on line %i", setting_name, setting_value, lineno);
  return 0;
}

static File configFile;

// This is a helper for reading config file -- it reads each line
static char* ini_reader_fcn(char* buffer, int max, void* stream) {
  // Note: can't coerce a void* stream into a File class, so using static var instead
  if (!configFile.available())
    return NULL;
  int len = configFile.readBytesUntil('\n', buffer, max);
  buffer[len] = '\0';
  return (char *) 1;
}

// Load settings from file
void programSettings::loadSettings()
{
  initSettings();
  configFile = LittleFS.open(SETTINGS_FILE);
  if (!configFile) {
    loge("Can't open configuration file '%s'", SETTINGS_FILE);
    return;
  }

  int line = ini_parse_stream(ini_reader_fcn, NULL, ini_handler_fcn, NULL);
  if (line) loge("Error in '%s' on line %i", SETTINGS_FILE, line);
  configFile.close();
  return;
}