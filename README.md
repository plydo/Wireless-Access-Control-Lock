
# Wireless Access Control Lock (WACL)

### Features:

* Controls a solenoid lock or relay
* Suitable for controlling access to:
  * a door via a solenoid lock 
  * a machine via an AC power relay
  * smaller devices like a box or a DC-powered device
* User display to show what's going on (optional)
* Access is controlled by an RFID reader
* Two access modes:
   * Scan to access (single access)
   * Scan to turn on access and scan again to turn off (continuous access)
* Access list controlled by a remote server via WiFi or (eventually) locally
* Setup and status via web browser via WiFi
   * Built-in web-accessible file server
* Logging
* Capable of handling a handful of users or thousands of users

### Hardware Requirements

* ESP32
* EM4100 rfid reader (eventually more options)
* A solenoid-activated lock or relay or something similar
* Power supply and lock/relay drive circuitry
* 1602 LCD (optional)
* Current sensor for machine control (optional but recommended)

### Setup

Initially, the WACL won't access a WiFi network. Instead, it will set up an
access point (AP) with SSID of "WACL-xxyyzz" where xxyyzz is random
letters/numbers.  Connect to this AP and then go to 192.168.4.1. Then,
update the file "config.txt" with the WiFi network's SSID, password, and other
settings. See "example_config.txt" for all of the settings.

---

This project is inspired by https://github.com/esprfid/esp-rfid