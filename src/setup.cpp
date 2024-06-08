// setup.cpp - one-time program startup/setup
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

#if ENABLE_DNS
#include <ESPmDNS.h>

static void setupMDNS()
{
  if (MDNS.begin(stg.hostName))
    logi("MDNS setup host 'http://%s.local'", stg.hostName);
  else
    loge("MDNS setup failed", stg.hostName);
}
#else
static void setupMDNS() {}
#endif

#if ENABLE_NTP
static void setupNTP()
{
  timeClientNTP.begin(); // uses "pool.ntp.org"
  bool success = timeClientNTP.forceUpdate();
  if (success) {
    setTime(timeClientNTP.getEpochTime());
    logi("NTP successful");
  } else {
    loge("NTP setup failed");
  }
  bootTime = now();
}
#else
void setupNTP() {}
#endif

/*
WiFi Access Point mode can be used for setting up the WiFi and other settings,
or it can be used standalone in this mode -- but without NTP date/time.
If WiFi doesn't initially connect, we can fall back to AP mode.
*/
static void setupWiFiAccessPoint()
{
  char ssid[40];

  sprintf(ssid, PROJECT_SHORT "-%02x%02x%02x", macAddr[3], macAddr[4], macAddr[5]);

  WiFi.mode(WIFI_AP);

  // optional: bool success = softAPConfig(local_ip, gateway, subnet, dhcp_lease_start=0);

  // params: ssid, password or def=NULL, channel (1-13, def=1), hidden
  //         (def=false), max_conn (1-4, def=4), ftm_responder (def=false)
  bool success = WiFi.softAP(ssid, NULL, 1, 0, 1, 0);
  if (success) logi("Set up '%s' Access Point", ssid); // note: the softAP fcn logs failure

  // Now connect your WiFi to this ap/ssid and navigate to http://192.168.4.1
  //   (this does not have a captive portal, at least not yet)
}

// This returns true if WiFi setup was successful and false if it failed
static bool setupWiFi()
{
  if (!*stg.wifiSSID) {
    logi("WiFi SSID configuration value is not set");
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(stg.wifiSSID, stg.wifiPassword);
  logi("Connecting to '%s' WiFi network", stg.wifiSSID);
  for (int i = 0; i < 12; i++) {
    lcd.print('.');
    delay(1000);
    if(WiFi.status() == WL_CONNECTED) {
      logi("WiFi connection established, IP %s", WiFi.localIP().toString().c_str());
      setupNTP();
      return true;
    } // if
  } // for
  Serial.println("");
  logw("Connecting to WiFi '%s' failed", stg.wifiSSID);
  return false;
}

static void setupLittleFS()
{
  if (!LittleFS.begin(/*formatOnFail =*/ false)) {
    if (!LittleFS.begin(/*formatOnFail =*/ true)) {
      loge("LittleFS initial mount failed, formatting failed");
    } else {
      logw("LittleFS initial mount failed, formatted successfully\r\n");
    }
  }
}

// This is executed by the Arduino framework once on startup/boot
void setup()
{
  bool WiFiModeAP;
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("\r\n--------------------------------------------------------------------------------");

  lcd.init();
  lcd.backlight();
  lcd.saveLine(0, PROJECT_SHORT " - V" VERSION);
  lcd.saveLine(1, "");
  lcd.printSaved();

  setupLittleFS();
  stg.loadSettings();

  WiFi.macAddress(macAddr); // set global var
  lock.stopAccess();
  lcd.setCursor(0, 1);
  lcd.print("WiFi");
  bool WiFiModeSta = true;
  if (!setupWiFi()) {
    setupWiFiAccessPoint();
    WiFiModeSta = false;
    WiFiModeAP = true;
  }
  // log here so that time is properly set by WiFi and NTP
  logw("BOOT: " PROJECT_SHORT " - " PROJECT_LONG ", V" VERSION ", built " __DATE__);
  setupMDNS();
  setupAsyncWebserver();
  webserverTimedout.reset(stg.webserverMinutes);

  uidAdmin.load(stg.adminIDs);
  // uidAdmin.zeroIndex();
  // logd("First Admin-ID: %lu", uidAdmin.next());

  if (stg.outputPin) pinMode(abs(stg.outputPin), OUTPUT);
  if (stg.beeperPin) pinMode(abs(stg.beeperPin), OUTPUT);
  if (stg.currentPin) pinMode(abs(stg.currentPin), INPUT);
  if (stg.voltagePin) pinMode(abs(stg.voltagePin), INPUT); // this may be the same pin as currentPin
  rdm6300.begin(stg.rx2Pin, stg.tx2Pin);
  lock.autoOffTimedout.disable();
  uidAdmin.adminTimedOut.disable();

  logd("Admin UN: '%s', PW: '%s'\r\n", stg.webserverUsername, stg.webserverPassword);
  serialInfo();

  lcd.setCursor(0, 1);
  if (WiFiModeSta) {
    lcd.print(WiFi.localIP());
    lcd.print('/');
    lcd.print(subnetIP());
  }
  if (WiFiModeAP) {
    lcd.print("AP ");
    lcd.print(WiFi.softAPIP());
  }
  lcd.print("      "); // clear end-of-line just in case
  // in a little while, replace ip address with boot date/time
  lcd.saveLine(1, formattedTime(localTime(bootTime), ftm_yyyymmddhhmm));
  lcd.setTimeout();
}