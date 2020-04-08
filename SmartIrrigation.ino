String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

#include <NTPClient.h>
//#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#include "credentials.h"

const char ssid[] = WIFI_SSID; //The name of the AP/SSID you connect to
const char pass[] = WIFI_PASSWD; //The WPA key of your AP

WiFiServer server(80);

bool offlineMode = true;
 
String request = "";
String requestTime = "";
int LED_Pin = D1;

//set both to the same value:
int valveCount = 5;
bool valveOn[5];

int timerValveNum = 5;

int tabLength = 10;
int entryCount = 0;

int startHistoryAtPosition = 100;
int startTimersAtPosition = 10;

// World time and date
const long utcOffsetInSeconds = 3600;
String formattedDate;
String dayStamp;
String timeStamp;
int timezoneOffset = +2;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// timer
int starttime;
unsigned long DELAY_TIME;
bool alreadyStarted = false;

// HTML files
String currentHtmlFile = "settings.html";
void setup() 
{
    pinMode(LED_Pin, OUTPUT);     
    Serial.begin(115200);

    // Connect to a WiFi network
    Serial.print(F("Connecting to "));  Serial.println(ssid);
    WiFi.begin(ssid, pass);
 
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
        if (offlineMode && millis() > (10*3600)) {
          Serial.println("No Wifi");
          break;
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println(F("[CONNECTED]"));
      Serial.print("[IP ");              
      Serial.print(WiFi.localIP()); 
      Serial.println("]");
    }
 
    // start a server
    server.begin();
    Serial.println("Server started");

    EEPROM.begin(512);
    
    // start NTP time client
    timeClient.begin();
    if (EEPROM.read(1) == 255)
        EEPROM.write(1, (timezoneOffset+12)*2); 
    else 
      timeClient.setTimeOffset(((EEPROM.read(1)  / 2 ) - 12) * 3600);
    //EEPROM.update();

    //timeClient.setTimeOffset(timezoneOffset * 3600);
   
    if (SPIFFS.begin())
        Serial.println(F("SPIFFS started."));
    else
        Serial.println(F("SPIFFS failed."));
    
    //AddSampleEntries();

    //RemoveAllTimers();
} // void setup()
 
 
 
void loop() 
{
      while(!timeClient.update()) {
        timeClient.forceUpdate();
      }
      //Serial.println("Time now: " + timeClient.getFormattedDate());
      //delay(20);
      CheckValves();


    CheckTimers();
  // The client will actually be disconnected when the function returns and 'client' object is detroyed
    for (int i =1; i <= valveCount; i++){
      if (valveOn[i] && (millis() - starttime) >= DELAY_TIME) {
          valveOn[i] = false; 
          Serial.print("Timer ended");
          alreadyStarted = false; /// needs fixing!
    }
  }
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client)  {  return;  }
 
    // Read the first line of the request
    request = client.readStringUntil('\r');
 
    if (request.indexOf("UPDATE") > 0) {}
    else {
      Serial.print("request: "); 
      Serial.println(request); 
    }
 
    if       ( request.indexOf("VALVEON") > 0 )  
             { 
              String t ="";
              int tStart = request.indexOf("d")+1;
              int tEnd = request.indexOf("m")-1;
              for (int i = tStart; i<=tEnd; i++){
                t.concat(request.charAt(i));
              }
              int n;
              int nStart = request.indexOf("m")+1;
                n = String (request.charAt(nStart)).toInt();
                
                Serial.println ("Time set to: " + t + " at: " + n);
                StartTimer ((t.toInt()), n);  
                client.print( header );
                client.print( "VALVEISON" );
             }
    else if  ( request.indexOf("VALVEOFF") > 0 ) 
             { 
                for (int i =1; i <= valveCount; i++){
                   valveOn[i] = false;
                }
                client.print( header );
                client.print( "VALVEISOFF"); 
             }
    else if  ( request.indexOf("SETTIMEZONE-") > 0 ) 
             { 
                int pos1 = request.indexOf("-")+1;
                int pos2 = request.indexOf("!")-1;
                String num = "";
                for (int i = pos1; i <= pos2; i++) {
                  num+= request.charAt(i);
                }
                Serial.println("TimeZone set to: " + num + "(" + ((num.toInt() + 12) * 2) + ")");
                Serial.println("Time now: " + timeClient.getFormattedDate());
                int t = (num.toFloat() + 12) * 2;
                EEPROM.write(1, t);
                EEPROM.commit();
                timeClient.setTimeOffset(t * 3600);
                client.print( header );
                //client.print( num ); 
             }

     else if  ( request.indexOf("RESETHISTORY") > 0 ) 
             { 
                AddSampleEntries();
                client.print( header );
                client.print( "SUCCESS"); 
             }
     else if  ( request.indexOf("SETHTMLFILE") > 0 ) 
             { 
                int durStart = request.indexOf("-")+1;
                int durEnd = request.indexOf("html")+3;
                String file = "";
                for (int i= durStart; i <= durEnd; i++) {
                  file += request.charAt(i);
                }
                currentHtmlFile = file;

                client.flush();
                client.print( header );
                Serial.println("Set Html to: " + runspiffs(currentHtmlFile));
                File f = SPIFFS.open(runspiffs(currentHtmlFile), "r");
                String content = f.readString();
                f.close();
                client.print(content);    
                delay(5);
                client.print( header );
                client.print("SUCCESS"); 
             }
    else if  ( request.indexOf("RESETTIMRS") > 0 ) 
             { 
                RemoveAllTimers();
                client.print( header );
                client.print( "SUCCESS"); 
             }
   else if  ( request.indexOf("UPDATE") > 0 ) 
             { 
              client.print( header );
              //Serial.println("sizeof valve: " + String(valveCount));
              for (int i =1; i <= valveCount; i++){
                if(valveOn[i] == true) {
                   client.print("Valve " + String(i) + " open now"); 
                   return;
                }
              }
              client.print("ValvesClosed"); 
             }
     else if  ( request.indexOf("TIME") > 0 ) 
             { 
                client.print( header );
                int ho = String (  String (request.charAt(request.indexOf("-")+1)) + String (request.charAt(request.indexOf("-")+2)) ).toInt();
                int mi = String (  String (request.charAt(request.indexOf("-")+3)) + String (request.charAt(request.indexOf("-")+4)) ).toInt();
                int durStart = request.indexOf("R")+1;
                int durEnd = request.indexOf("!")-1;
                String dur = "";
                for (int i= durStart; i <= durEnd; i++) {
                  dur += request.charAt(i);
                }
                int du = dur.toInt();
                String valv = String(request.charAt(durEnd+2));
                int v = valv.toInt();
                SetTime(ho, mi, du, v);
                client.print( "SUCCESS" ); 
                Serial.println("I was at TIME " + PutPrefix(mi) + ":" + PutPrefix(ho) + "-" + String(du) );
             }
      else if  ( request.indexOf("DELETETIMR") > 0 ) 
             { 
                client.print( header );
                int pos = request.indexOf("-")+1;
                String s = "";
                for (int i = pos; i < pos+10; i++) {
                   s += request.charAt (i);
                }
                //Serial.println("String is" + s);
                
                for (int i = startTimersAtPosition; i < startTimersAtPosition + 60; i = i + 6) {
                  String seeprom = PutPrefix(EEPROM.read(i)) +  PutPrefix(EEPROM.read(i+1)) + PutPrefix(EEPROM.read(i+2)) + PutPrefix(EEPROM.read(i+3)) + PutPrefix(EEPROM.read(i+4));
                  if (seeprom== s ){
                    Serial.println( "Timer Removed :" + s);
                    RemoveTimer(i);
                    break;
                  }
                }
                client.println("SUCCESS");
             }
       else if  ( request.indexOf("SETTIMRACTIVATED") > 0 ) 
             { 
                client.print( header );
                int pos = request.indexOf("-")+1;
                String s = "";
                for (int i = pos; i < pos+10; i++) {
                   s += request.charAt (i);
                }
                bool b;
                String boo = String (request.charAt(pos+10));
                if (boo == "f") {
                  b = false;}
                else if (boo == "t") {
                  b = true;}

                //Serial.println("String is" + s);
                
                for (int i = startTimersAtPosition; i < startTimersAtPosition + 60; i = i + 6) {
                  String seeprom = PutPrefix(EEPROM.read(i)) +  PutPrefix(EEPROM.read(i+1)) + PutPrefix(EEPROM.read(i+2)) + PutPrefix(EEPROM.read(i+3)) + PutPrefix(EEPROM.read(i+4));
                  if (s  ==  seeprom ){
                    if (b) {
                       EEPROM.write(i+5, 1);
                       client.println("SUCCESS");
                       Serial.println("Timer activated");
                       break;
                       }
                    else {
                       EEPROM.write(i+5, 0);
                       client.println("SUCCESS");
                       Serial.println("Timer deactivated");                       
                       break;
                    }
                  }
                }
                EEPROM.commit();
             }
    else if  ( request.indexOf("INIT") > 0 ) 
             { 
                client.print( header );
                String response = "+";
                for (int i = startHistoryAtPosition; i < startHistoryAtPosition + (tabLength * 7); i++) {
                  if (EEPROM.read(i) < 10) {
                    response += "0";
                    response += String ( EEPROM.read(i) );
                  }
                  else {
                    response += String ( EEPROM.read(i) );
                  }
                  response += " ";
                }
                response += "-";
                for (int i = startTimersAtPosition; i < startTimersAtPosition + 60; i = i + 6) {
                  String r = "";
                  if (EEPROM.read(i+3) != 0 || EEPROM.read(i+2) != 0){
                    for (int j = 0; j < 6; j++) {
                      r += PutPrefix(EEPROM.read(j+i));
                      r += " ";
                    }
                  }
                  response += r;
                }
                response = (response);
                client.print( response ); 
             }     
   else if  ( request.indexOf("INSETTINGS") > 0 ) 
             { 
                client.print( header );
                float i = EEPROM.read(1);
                Serial.println("Read :" +  String(i) + "(" + String(( i / 2) -12) + ")");
                client.println(String(( i / 2) -12)); 
             }
    else
    {
        client.flush();
        client.print( header );
        Serial.println("At else: " + runspiffs("index.html"));
        File f = SPIFFS.open(runspiffs("index.html"), "r");
        String content = f.readString();
        f.close();
        client.print(content);    
        delay(5);
    }

} // void loop()

void StartTimer(int t, int valve) {
    Serial.println ("Timer Started");
    starttime = millis();
    valveOn[valve] = true;
    DELAY_TIME = t * 1000;
    AddEntry (startHistoryAtPosition, t, valve);
}

void CheckTimers() {
    for (int i = startTimersAtPosition; i < startTimersAtPosition + 60; i = i + 6) {
      if (EEPROM.read (i+5) != 0) {
        if (EEPROM.read (i) == timeClient.getHours() ) {
          if (EEPROM.read (i+1) == timeClient.getMinutes() ) {
            if (timeClient.getSeconds()  == 0) {
              if (!alreadyStarted) {
                  String d = String (EEPROM.read (i+2) ) + String (EEPROM.read (i+3) );
                  StartTimer(d.toInt(), EEPROM.read (i+4));
                  alreadyStarted = true;
              }
            }  
          }
        }
      }
    }
    //delay(5);
}

String getFileString(String fileName) {
    File stylecss = SPIFFS.open(fileName, "r");
    if (!stylecss) {
      Serial.println("Css Datei nichtgefunden");
      return "";
    }
    else if(stylecss) {
      Serial.println("file Opened");
      Serial.println ("File Content: " + stylecss.readString());
      return stylecss.readString();
  }
}

void CheckValves(){
  for (int i =1; i <= valveCount; i++){
     if (valveOn[i]){
        SetValve(i, true);
     }
     else {
       SetValve(i, false);
     }
  }
}

void AddSampleEntries() {
  for (int i = startHistoryAtPosition; i < startHistoryAtPosition + (tabLength*7); i = i+7) {
   AddEntry(i, 5, 5);
  } 
}

void SetValve (int valveNum, bool state) {
  if (state == false){
    digitalWrite(valveNum, LOW);
    valveOn[valveNum] = false;
  }
  else{
    digitalWrite(valveNum, HIGH);
    valveOn[valveNum] = true;
  }
}

//int i 1 for hour, 2 for minute,
String GetTimestamp(int mod) {
  formattedDate = timeClient.getFormattedDate();

  int splitT = formattedDate.indexOf("T");
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  timeStamp.replace (":", "");
  if (mod == 1) {
    timeStamp = String(timeStamp.charAt(0))  + String(timeStamp.charAt(1));
  }
  else if (mod == 2) {
    timeStamp = String(timeStamp.charAt(2))  + String(timeStamp.charAt(3));
  }
  return (timeStamp);
}

//int i 1 for 20year, 2 for month, 3 for day 
String GetDatestamp(int mod) {
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  dayStamp.replace ("-", "");
  if (dayStamp.indexOf("Z") > 0){
      dayStamp.replace ("Z", "");
  }

  if (mod == 1) {
      dayStamp = String(dayStamp.charAt(2))  + String(dayStamp.charAt(3));
    }
  else if (mod == 2) {
    dayStamp = String(dayStamp.charAt(4))  + String(dayStamp.charAt(5));
  }
  else if (mod == 3) {
    dayStamp = String(dayStamp.charAt(6))  + String(dayStamp.charAt(7));
  }
   return (dayStamp);
}

String runspiffs(String Fname) {
 
    // To format all space in SPIFFS
    //SPIFFS.format();
 
    // Get all information of your SPIFFS
    FSInfo fs_info;
    SPIFFS.info(fs_info);
  
    // Open dir folder
    Dir dir = SPIFFS.openDir("/");

    dir = SPIFFS.openDir("/");
    while (dir.next()) {
            
        if(dir.fileSize()) {
            File f = dir.openFile("r");
            f.close();
        }else{
            Serial.println("0 bytes");
        }
      
        if(dir.fileName().indexOf(Fname) > 0){
          return dir.fileName();
        }

    }

}

//add entry to history
void AddEntry(int location, int duration, int valve) {
  int duration_limited;
  if (duration < 99)
    duration_limited = duration;
  else
    duration_limited = 99;
    
  if (EEPROM.read(location) > 0){
    MoveEntries();
    Serial.println("Move entries");
  }
    EEPROM.write((location), GetTimestamp(1).toInt()); // hour
    EEPROM.write((location)+1, GetTimestamp(2).toInt());  // minute
    EEPROM.write((location)+2, GetDatestamp(1).toInt());  // 20year
    EEPROM.write((location)+3, GetDatestamp(2).toInt());  // month
    EEPROM.write((location)+4, GetDatestamp(3).toInt());  // day
    EEPROM.write((location)+5, valve);  // valve
    EEPROM.write((location)+6, duration_limited);  // duration
    EEPROM.commit();
    Serial.println ("New entry at location: " + String(location));
}

//Set new timer
void SetTime(int hour, int minute, int duration, int valve) {
  int dur1, dur2;
  if (duration < 100) {
     dur1=0;
     dur2 = duration;
  }
  else {
    dur1 = duration / 100;
    dur2 = duration % 100;
  }
  for (int i = startTimersAtPosition; i <startTimersAtPosition + 60; i= i + 6){
    if (EEPROM.read(i) == 0){      
      EEPROM.write(i, hour);
      EEPROM.write(i + 1, minute);
      EEPROM.write(i + 2, dur1);
      EEPROM.write(i + 3, dur2); 
      EEPROM.write(i + 4, valve);  //set valve valve
      EEPROM.write(i + 5, 1);  //set timer active
      break;
    }
  }
  EEPROM.commit();
  
  Serial.println("print out timers");
    for (int i = startTimersAtPosition; i <startTimersAtPosition + 60; i= i + 6){
      for (int j = 0; j<6; j++)
        Serial.print (String (EEPROM.read(i+j)) + String (" "));
      Serial.println();
  }
}

void RemoveAllTimers() {
  for (int i = startTimersAtPosition; i <startTimersAtPosition + 60; i++){
     EEPROM.write(i, 0);
  }
}

void RemoveTimer(int location) {
  for (int i= location; i< location+6; i++) {
      EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void MoveEntries() {
  for (int h = startHistoryAtPosition + tabLength - 1; h >= startHistoryAtPosition; h = h - 7) {
    for (int g = 0; g < 7; g++){
      EEPROM.write(h+g, EEPROM.read( h+g-7 ));
    }
    EEPROM.commit();
    }
}

String PutPrefix (int num) {
  if (num < 10) {
    return("0"+String(num));
  }
  else {
    return( String (num) );
  }
}
