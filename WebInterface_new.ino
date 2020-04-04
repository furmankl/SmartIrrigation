String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
String header2 = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n";

#include <NTPClient.h>
//#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// change these values to match your network
char ssid[] = "VitaKlemen";       //  your network SSID (name)
char pass[] = "rjavikujn25";      //  your network password
 
WiFiServer server(80);
 
String request = "";
String requestTime = "";
int LED_Pin = D1;

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

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// timer
int starttime;
unsigned long DELAY_TIME;
bool alreadyStarted = false;

void setup() 
{
    pinMode(LED_Pin, OUTPUT);     
 
    Serial.begin(115200);

 
    // Connect to a WiFi network
    Serial.print(F("Connecting to "));  Serial.println(ssid);
    WiFi.begin(ssid, pass);
 
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(200);
    }
 
    Serial.println("");
    Serial.println(F("[CONNECTED]"));
    Serial.print("[IP ");              
    Serial.print(WiFi.localIP()); 
    Serial.println("]");
 
    // start a server
    server.begin();
    Serial.println("Server started");

    timeClient.begin();
    timeClient.update();
    EEPROM.begin(512);

    if (SPIFFS.begin()){
        Serial.println(F("done."));
    }else{
        Serial.println(F("fail."));
    }    
    timeClient.setTimeOffset(7200);
     //AddSampleEntries();
    
    Serial.println ("Time now: " + GetTimestamp(1) + ":" + GetDatestamp(2));
     
  for (int i = startTimersAtPosition; i <startTimersAtPosition + 50; i++){
     if (EEPROM.read(i) > 0){}
     else {
      break;
     }
  }

 

  //RemoveAllTimers();
} // void setup()
 
 
 
void loop() 
{
      timeClient.update();
      CheckValves();


    CheckTimers();
  // The client will actually be disconnected when the function returns and 'client' object is detroyed
    for (int i =0; i <= sizeof(valveOn); i++){
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
 
    Serial.print("request: "); Serial.println(request); 
 
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
                StartTimer ((t.toInt())*1000, n);  
                client.print( header );
                client.print( "VALVEISON" );
             }
    else if  ( request.indexOf("VALVEOFF") > 0 ) 
             { 
                for (int i =0; i <= sizeof(valveOn); i++){
                   valveOn[i] = false;
                }
                client.print( header );
                client.print( "VALVEISOFF"); 
             }
    else if  ( request.indexOf("RESETTIMERS") > 0 ) 
             { 
                RemoveAllTimers();
                client.print( header );
                client.print( "SUCCESS"); 
             }
   else if  ( request.indexOf("GETCSS") > 0 ) 
             { 
              client.print( header2 );
                //int strStart = request.indexOf("-")+1;
                //int strEnd = request.indexOf("!THEEND!")-1;
                //String fileName ="/";
                //for (int i = strStart; i <= strEnd;i++ ) {
                 // fileName += request.charAt(i);
                //}
                String fn = runspiffs("css");
                Serial.println("File Name: " + fn);
                File f;
                f = SPIFFS.open(fn, "r");                
                String cssContent =  f.readString();
                cssContent.replace ("\n", "");
                Serial.println("File content: " + cssContent);   
                client.print(cssContent); 
                f.close();
             }
     else if  ( request.indexOf("TIME") > 0 ) 
             { 
                digitalWrite(LED_Pin, LOW);   
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
                SetTime(ho, mi, du);
                client.print( "SUCCESS" ); 
                Serial.println("I was at TIME " + PutPrefix(mi) + ":" + PutPrefix(ho) + "-" + String(du) );
             }
      else if  ( request.indexOf("DELETETIMR") > 0 ) 
             { 
                digitalWrite(LED_Pin, LOW);   
                client.print( header );
                int pos = request.indexOf("-")+1;
                String s = "";
                for (int i = pos; i < pos+10; i++) {
                   s += request.charAt (i);
                }
                //Serial.println("String is" + s);
                
                for (int i = startTimersAtPosition; i < startTimersAtPosition + 50; i = i + 5) {
                  String seeprom = PutPrefix(EEPROM.read(i)) +  PutPrefix(EEPROM.read(i+1)) + PutPrefix(EEPROM.read(i+2)) + PutPrefix(EEPROM.read(i+3)) + PutPrefix(EEPROM.read(i+4));
                  if (seeprom== s ){
                    Serial.println( "Timer Removed :" + s);
                    RemoveTimer(i);
                    break;
                  }
                }
                client.println("SUCCESS" );
             }
    else if  ( request.indexOf("INIT") > 0 ) 
             { 
                digitalWrite(LED_Pin, LOW);   
                client.print( header );
                String response = "";
                for (int i = startHistoryAtPosition; i < startHistoryAtPosition + (tabLength * 6); i++) {
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
                for (int i = startTimersAtPosition; i < startTimersAtPosition + 50; i = i + 5) {
                  String r = "";
                  if (EEPROM.read(i+4) != 0){                
                    for (int j = 0; j < 5; j++) {
                      if (EEPROM.read(j+i) < 10) {
                        r += "0";
                        r += String ( EEPROM.read(j+i) );
                      }
                      else {
                        r += String ( EEPROM.read(j+i) );
                      }
                        r += " ";
                    }
                  }
                  response += r;
                }
                response = ("+" + response);
                client.print( response ); 
             }             
    else
    {
        client.flush();
        client.print( header );
        Serial.println("At else: " + runspiffs("html"));
        File f = SPIFFS.open(runspiffs("html"), "r");
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
    DELAY_TIME = t;
    AddEntry (startHistoryAtPosition);
}

void CheckTimers() {
    for (int i = startTimersAtPosition; i < startTimersAtPosition + 50; i = i + 5) {
      if (EEPROM.read (i+4) != 0) {
        if (EEPROM.read (i) == timeClient.getHours() ) {
          if (EEPROM.read (i+1) == timeClient.getMinutes() ) {
            if (timeClient.getSeconds()  == 0) {
              if (!alreadyStarted) {
                  String d = String (EEPROM.read (i+2) ) + String (EEPROM.read (i+3) );
                  StartTimer(d.toInt(), timerValveNum);
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
  for (int i =0; i <= sizeof(valveOn); i++){
     if (valveOn[i]){
        SetValve(i, true);
     }
     else {
       SetValve(i, false);
     }
  }
}
void AddSampleEntries() {
  for (int i = startHistoryAtPosition; i < startHistoryAtPosition + (tabLength*6); i = i+6) {
   AddEntry(i);
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

String runspiffs(String typ) {
 
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
      
          if(dir.fileName().indexOf("css") > 0 && typ == "css"){
          return dir.fileName();
        }
        else if(dir.fileName().indexOf("html") > 0 && typ == "html"){
          return dir.fileName();
        }

    }

}

void AddEntry(int location) {
      timeClient.update();

  if (EEPROM.read(location) > 0){
    MoveEntries();
  }
    EEPROM.write((location), GetTimestamp(1).toInt()); // hour
    EEPROM.write((location)+1, GetTimestamp(2).toInt());  // minute
    EEPROM.write((location)+2, GetDatestamp(1).toInt());  // 20year
    EEPROM.write((location)+3, GetDatestamp(2).toInt());  // month
    EEPROM.write((location)+4, GetDatestamp(3).toInt());  // day
    EEPROM.write((location)+5, 2);  // date
    EEPROM.commit();
    Serial.println ("New entry at location: " + String(location));
}

void SetTime(int hour, int minute, int duration) {
  int dur1, dur2;
  if (duration < 100) {
     dur1=0;
     dur2 = duration;
  }
  else {
    dur1 = duration / 100;
    dur2 = duration % 100;
  }
  for (int i = startTimersAtPosition; i <startTimersAtPosition + 50; i= i + 5){
    if (EEPROM.read(i) == 0){      
      EEPROM.write(i, hour);
      EEPROM.write(i + 1, minute);
      EEPROM.write(i + 2, dur1);
      EEPROM.write(i + 3, dur2); 
      EEPROM.write(i + 4, 1);  //is Timer active?
      break;
    }
  }
  EEPROM.commit();
  
  Serial.println("print out timers");
    for (int i = startTimersAtPosition; i <startTimersAtPosition + 50; i= i + 5){
      for (int j = 0; j<4; j++)
        Serial.print (String (EEPROM.read(i+j)) + String (" "));
      Serial.println();
  }
}
void RemoveAllTimers() {
  for (int i = startTimersAtPosition; i <startTimersAtPosition + 50; i++){
     EEPROM.write(i, 0);
  }
}
void RemoveTimer(int location) {
  for (int i= location; i< location+5; i++) {
      EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void MoveEntries() {
  for (int h = startHistoryAtPosition; h <= tabLength + startHistoryAtPosition; h= h + 6) {
    for (int g = 1; g <=6; g++){
      EEPROM.write(h+6+g, EEPROM.read( h+g ));
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
