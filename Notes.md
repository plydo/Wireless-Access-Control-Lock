------------------------------------------------------------------------------------------
### Notes, Change Log, To-Do, etc. for the Wireless Access Control Lock (WACL) project ###
------------------------------------------------------------------------------------------
# MISC NOTES
Speed (ESP32-D0WDQ6-1 @ 240 MHz):
  About 1000-1200 ms https response independent of body size from <100 to ~10,000 bytes
  Processing 9974 bytes, 554 json keys, only id(text)+active (18 bytes/key), is 200-210 ms
Flash (4 Mb total):
  Program:     2560 kb Total (2x1280), 1054 kb (82%) Used, Filesystem: 1408 kb - default partitions
  Program:     3584 kb Total (2x1792), 1061 kb (59%) Used, Filesystem:  384 kb - 2024-05-27
  Program:     1072 kb (1092157) - 2024-06-01 program changes, lib update (ref "pio pkg outdated"),
    did "pio pkg update", updated: espressif32 6.5 -> 6.7, caused arudionespressif32 -> 3.20016.0
Memory (RAM/Heap, 320 kb total):
  Initial observation: about 100k ram usage, 30k for cache, 60k temp, no visible fragmentation
  Heap (kb):   174 initial, 132 (175) current,  61 lowest, 107 largest block after dump-active test
  Heap (kb):   174 initial, 162 (206) current, 119 lowest, 107 largest block after single entry
  Heap (kb):   174 initial, 164 (207) current, 161 lowest, 107 largest block before single entry
  Heap (kb):   172 initial, 160 (203) current, 117 lowest, 107 largest block 2024-05-27 after awhile

RDM6300:
  For the rdm6300 125 kHz RFID reader, a 5+ mm distance (wall thickness) between the coil and
  the key fob works better than a <3 mm distance. If it's too close, it won't read the key fob.

------------------------------------------------------------------------------------------
# CHANGELOG
2024-02-12 Started
  ...
  changed partition table (.csv) to increase program space and decrease filesystem space
    was 2x1280 kb program, 1408 kb filesystem, 3568 kb pgm+fs
    now 2x1792 kb program,  384 kb filesystem, 3568 kb pgm+fs
  Allow voltage pin to be a different pin instead of always combined with the current pin
  Added logging of users, initially just local logging
    Added logging to file with file size limited by the value of a config variable
    Added the last few lines of the log file to the home/info webpage
    Added separate config variables for file logging level & serial logging level (for serial debug)
  Added admin mode for adding authorized users by scanning their key fob
    Added a config variable for a list of "admin" ID's
    Added an object containing variables and methods for the admin ID's
    Added code to detect two admin id scans within a few seconds to trigger the new mode
      Entering the admin id again disables the admin mode
      A timeout also auto-disables the admin mode
  Require webserver authentication/password for everything except home/root/index
  Added webserver auto-disable timeout - webserverMinutes
    Could (but didn't) re-enable if admin-ID is scanned -- use negative webserverMinutes?
2024-05-12 Released as v1.0
2024-06-08 Re-released as v1.0 and put on github

------------------------------------------------------------------------------------------
# TODO
Change usage minutes from machine-enabled time to current-drawn time
Add support for no display -- maybe it already works that way?
Show date/time on LCD for pulse mode?
Add caching option(s) -- see below CACHING comments
Add option to monitor current and set level thru analog input -- must analyze 60 Hz signal
Add timezone to settings
 - Time-zone-minutes
 - Time-change = mon,wk,day,hr,offset x2, scan #'s w/ strtoul() or strtol()
Add local "back end"
Add support for readerType of MFRC522, Wiegand (wgd0pin,wgd1pin), PN532
    for MFRC522 RFID reader, setting for Antenna Gain - rfidgain
Add beeper support

Someday maybe...
Test/operate relay activation from webpage?
Add option to manually operate it via switch input on a pin (like esp-rfid)?
MQTT? (for logs? for config from server?)
Add pressure sensor for dust collection interlock?
Add to web interface a display of ALL settings,
  including ones not in config file that were left as default?
Add support to download files instead of just copy/paste?
  Check out sometime: https://github.com/G6EJD/G6EJD-ESP-File-Server
Select Wifi netwwork by doing a scan & selecting from a list, instead of manually configure?
---
The listDir() fcn has code for "levels" (subdirectories) but the code doesn't work
  Not very important since we don't use subdirs

------------------------------------------------------------------------------------------
# TODO - CACHING OF IDs
Caching Strategies
1) Cache everything needed and check backend only if cache hit fails
   - update cache periodically (multitask to avoid UI delay?)
   - update cache continuously via backend push?
   - good to notify if cache updates continuously fails
2) Always check backend and use cache only if backend fails
   - cache everything needed (per methods described above)
   - partial cache derived from previous backend checks - PREFERRED?
     - could purge unused old cache entries
     - could periodically check cached entries so they stay up-to-date
3) Do not cache -- always check backend - simple
Cache Location
1) RAM
2) Filesystem
   - store id's plaintext - easy, maintainable, not secure
   - store id's salted hash (1-way fcn) - hard, secure, can never see id list again
Considerations
1) Backend data access speed impacts choice of strategy
2) If caching all data is large, using the Filesystem, not RAM, may be required
    Though 20 bytes/entry (16 char for name + 4 bytes for ID) is 10kb for 500 entries
      so RAM size may not too bad, but watch heap size reporting

------------------------------------------------------------------------------------------
# LIBRARIES/MODULES/SAMPLE-CODE USED
  FS - LittleFS - BSD 3 License - included in the Arduino esp32 board pkg
  FreeRTOS - MIT License - included in the Arduino esp32 board pkg
  OTA - basic features are included in the Arduino esp32 board pkg
    For code examples, search github etc for "OTA Web Updater"
    Webserver-initiated OTA updates are included in the Web File Manager (below)
  NTP Time - MIT License - used arduino-libraries/NTPClient@^3.2.1
  UTC-to-Local-Time - GPL-3.0 License - used Jchristensen/Timezone
  Time/TimeLib - included by NTP and/or UTC/Timezone
  WebServer - LGPL License - used me-no-dev/ESPAsyncWebServer (ESP Async WebServer)
  AysyncTcP - included by me-no-dev/ESPAsyncWebServer
  Web File Manager - used me-no-dev/ESPAsyncWebServer and code from:
    https://www.hackster.io/myhomethings/esp32-web-updater-and-spiffs-file-manager-cf8dc5
    (Another interesting one is: https://github.com/G6EJD/G6EJD-ESP-File-Server)
    (No License, No Copyright)
  INI (config file) Parser - BSD License - using https://github.com/benhoyt/inih/
    Also looked at https://github.com/compuphase/minIni/
  Json - MIT License - bblanchon/ArduinoJson
  Display - 16x2 (1602) character LCD - No License - using njoyneering/LiquidCrystal_I2C

------------------------------------------------------------------------------------------
# OTHER REFERENCE INFO
  Libraries That Were Looked At
    Https Webserver (self-signed?) (for better security than http)
      There's fhessel/esp32_https_server but it doesn't support forms or templates so not useful
    WiFi Manager - just fallback to AP mode & let the user configure WiFi there
      WiFi configuration can be done using features in the Web File Manager (below)
      Commonly used WiFi Managers:
      - khoih-prog/ESPAsync_WiFiManager
      - tzapu/WiFiManager
  Other Docs
    https://randomnerdtutorials.com/esp32-https-requests/
    https://randomnerdtutorials.com/esp32-esp8266-https-ssl-tls/#ssl-certificates
  Other Components
    https://github.com/mobizt/ESP-Google-Sheet-Client   Google Sheets w/ Arduino framework
      ESP-Google-Sheet-Client by Mobizt
        Ref: https://randomnerdtutorials.com/esp32-datalogging-google-sheets/
        Src: https://github.com/mobizt/ESP-Google-Sheet-Client/tree/master
        ISSUE:
        mobizt/ESP-Google-Sheet-Client uses FirebaseJson but everyone else uses ArduinoJson,
          so port the former to the latter? First impression is this looks like a big library.
    https://github.com/artem-smotrakov/esp32-weather-google-sheets?tab=readme-ov-file
      Another maybe lighter code base, but written in MicroPython, also it's only logging
    https://github.com/cotestatnt/Arduino-Google-API
      Another one
  Other frameworks/webservers/examples/etc
    https://github.com/onewithhammer/ESP8266-MyWidget-Demo bigger & more complicated
    https://github.com/cwi-dis/iotsa small, simple
    https://github.com/ArveLarve/ESPBase very small, ota & wifi-mgr, uses html in littlefs
    https://github.com/lasselukkari/aWOT
    https://github.com/har-in-air/ESP32_ASYNC_WEB_SERVER_SPIFFS_OTA -- pretty nice !!!
    https://github.com/G6EJD/G6EJD-ESP-File-Server

------------------------------------------------------------------------------------------
# NOTES ON PLATFORMIO.INI FILE
The Platformio Libraries feature overwrites the platformio.ini file. It preserves added
settings but removes added comments. So platformio.ini related info is documented here...

As of early 2024, "me-no-dev/ESP Async WebServer" (the latest stable version)
  doesn't work because of an issue with undefined mbedtls_md5_starts' as discussed here:
    https://github.com/philbowles/ESPAsyncWebServer/issues/3
    https://github.com/me-no-dev/ESPAsyncWebServer/issues/1147
  One solution to this issue is to use the latest development version:
      "https://github.com/me-no-dev/ESPAsyncWebServer.git"
    in place of "me-no-dev/ESP Async WebServer"
