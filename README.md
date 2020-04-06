# SmartIrrigation
==================

Simple ESP8266 Smart Irrigation system with bootstrap web interface

Requirements
------------

This was built on an "LOLIN D1 mini" with a Relay shield. Here is the  [Aliexpress link to the 3 € Wemos board ](https://www.aliexpress.com/item/32529101036.html?spm=a2g0s.9042311.0.0.27424c4d6wPAsC) and the [link to the € 0,39 relay shield](https://www.aliexpress.com/item/32737849680.html?spm=a2g0s.9042311.0.0.27424c4d6wPAsC).


Installation
------------

To build and install it, I use the Arduino IDE software.

You'll need the following libraries in Arduino IDE:
1. ESP8266 board libraries
2. Wifi (from Arduino, install from the Library Manager)
3. SPIFFS
4. ... 

And the following library, installed from git repository (!)
1. NTP Cleint -  [taranais's fork](https://github.com/taranais/NTPClient) (download master as .zip an extract it to libraries folder)

Usage
-----

The main functionality of Arduino-irrigation is that you can set the timers in a 
web interface, determining the time, duration and select valve (relay) to open. It also 
supports constant triggering of the relays. Additionally, all triggerings are saved to 
the board flash storage with SPIFFS. 
Code opens SPIFFS storage with 512 locations (each 1 byte) and is later assigned following
data (locations can be changed)
| Location  | Data |
| ------------- | ------------- |
| 0 - 10  | Essential data  |
| 10 - 80  | Place for timers data (max 10 timers)  |
| 100 - 170  | Recent valve triggers (now last 10 triggerings, can be changed)  |

To-do
-----
- finish settings page
- enable off-line function (without wifi connection)
- ...
