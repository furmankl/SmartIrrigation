# SmartIrrigation

Simple ESP8266 (LOLIN D1 mini) Smart Irrigation system with bootstrap web interface

Requirements
------------

This was built on an "LOLIN D1 mini" with a Relay shield. Here is the  [Aliexpress link to the 3 € Wemos board ](https://www.aliexpress.com/item/32529101036.html?spm=a2g0s.9042311.0.0.27424c4d6wPAsC) and the [link to the € 0,39 relay shield](https://www.aliexpress.com/item/32737849680.html?spm=a2g0s.9042311.0.0.27424c4d6wPAsC).

<img src="https://i.ibb.co/FXxLfX0/92104625-2993948287334472-6268575034176962560-n.jpg" height="300">
Image 1: Lolin D1 Mini with 12V DC Power Shield and a Relay Shield.

Installation
------------
### Libraries
To build and install it, I use the Arduino IDE software.

You'll need the following libraries in Arduino IDE:
1. ESP8266 board libraries
2. Wifi (from Arduino, install from the Library Manager)
3. ... 

And the following library, installed from git repository (!)
1. NTP Cleint -  [taranais's fork](https://github.com/taranais/NTPClient) (download master as .zip an extract it to libraries folder)
2. Arduino ESP8266 filesystem uploader (SPIFFS) [availabe in this repo](https://github.com/esp8266/arduino-esp8266fs-plugin) (download it from releases and follow the instructions on github readme or follow [this guide](https://www.instructables.com/id/Using-ESP8266-SPIFFS)  

### Set-up
1. Use Spiffs to upload files from the "data" folder to the board's flash memory (if unclear, follow [this guide](https://www.instructables.com/id/Using-ESP8266-SPIFFS/)  
2. Change wifi credentials in your .ino files
3. On a computer that is on the same LAN network as the board, open WebView in the brower at the boards's IP adress (will probably be something like 192.168.1.x, can be found in Serial feed or in your Router settings)
4. Set new Timers in WebView or use it to immidiately trigger relays

<img src="https://i.ibb.co/QN0SSHh/SIrrigation-webview.png" height="300">
Image 2: SIrrigation Web View, based on the "Purple Admin" Bootstrap template from www.bootstrapdash.com

Usage
-----

The main functionality of Arduino-irrigation is that you can set the timers in a 
web interface, determining the time, duration and select valve (relay) to open. It also 
supports constant triggering of the relays. Additionally, all triggerings are saved to 
the board flash storage with SPIFFS. SIrrigation works offine, however it needs to be connected
to Wi-fi when started in order to work as intended (I suggest making a hotspot on a mobile phone 
if there is no Wi-Fi available, so time is set. Wi-Fi can be later broken. 
Code opens SPIFFS storage with 512 locations (each 1 byte) and is later assigned following
data (locations can be changed)
| Location  | Data |
| ------------- | ------------- |
| 0 - 10  | Essential data  |
| 10 - 80  | Place for timers data (max 10 timers)  |
| 100 - 170  | Recent valve triggers (now last 10 triggerings, can be changed)  |



To-do
-----
- Add new settings to settings page
- enable off-line function (set time from settings)
- add moisture sensor capability
