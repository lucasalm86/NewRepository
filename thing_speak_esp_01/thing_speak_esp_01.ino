/*
 * HTTP Client
 * Copyright (c) 2015, circuits4you.com
 * All rights reserved.
/* Connects to WiFi HotSpot. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

//=======================================================================
//                    Data Recieved from Arduino Setup
//=======================================================================
//char receivedChars[numChars]; // an array to store the received data
String strMessageFromCOM, Co2, PM2_5, PM10, Temp, Hum, Pres;
boolean newData = false;

/* Set these to your desired credentials. */
const char *ssid = "Xia_n2";  //ENTER YOUR WIFI SETTINGS <<<<<<<<<
const char *password = "Envi32Envi32";

//Web address to read from
const char *host = "api.thingspeak.com";
String apiKey = "387JP60RMA4TDJO6";  //ENTER YOUR API KEY <<<<<<<<<<<
//=======================================================================
//                    Power on setup
//=======================================================================



void setup() {
  delay(1000);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);        //This line hides the viewing of ESP as wifi hotspot
  //WiFi.mode(WIFI_AP_STA);   //Both hotspot and client are enabled
  //WiFi.mode(WIFI_AP);       //Only Access point
  
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
}

//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {
  WiFiClient client;          
  const int httpPort = 80; //Port 80 is commonly used for www
 //---------------------------------------------------------------------
 //Connect to host, host(web site) is define at top 
 if(!client.connect(host, httpPort)){
   Serial.println("Connection Failed");
   delay(300);
   return; //Keep retrying until we get connected
 }
// recieving data from Arduino
 recvWithEndMarker();
 if (getValue(strMessageFromCOM, ':', 0) == "Arduino")
  {
    showNewData();
    //showNewData();
    strMessageFromCOM = "";
  }
//---------------------------------------------------------------------
  //Make GET request as pet HTTP GET Protocol format
  String ADCData;
  int adcvalue=analogRead(A0);  //Read Analog value of LDR
  ADCData = String(adcvalue);   //String to interger conversion
  String Link="GET /update?api_key="+apiKey+"&field1=";  //Requeste webpage  
  Link = Link + ADCData;
  Link = Link + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n";                
  client.print(Link);
  delay(100);
  
//---------------------------------------------------------------------
 //Wait for server to respond with timeout of 5 Seconds
 int timeout=0;
 while((!client.available()) && (timeout < 1000))     //Wait 5 seconds for data
 {
   delay(10);  //Use this with time out
   timeout++;
 }

//---------------------------------------------------------------------
 //If data is available before time out read it.
 if(timeout < 500)
 {
     while(client.available()){
        Serial.println(client.readString()); //Response from ThingSpeak       
     }
 }
 else
 {
     Serial.println("Request timeout..");
 }

 delay(5000);  //Read Web Page every 5 seconds
}
//=======================================================================

//---------------------------------------------------------------------
 //recieving data functions
void recvWithEndMarker()
{
  String str = "";

  if (Serial.available() > 0) {
    //Serial.println("Arduino getting string... " +  String(newData));
    strMessageFromCOM = Serial.readStringUntil('\n');
    delay(10);
    newData = true;
  }
}

void showNewData() {
  if (newData == true) {
    //Serial.println("ESP answer... ");
    //Arduino:890:5:6:25:60:990
    Co2 = getValue(strMessageFromCOM, ':', 1);
    PM2_5 = getValue(strMessageFromCOM, ':', 2);
    PM10 = getValue(strMessageFromCOM, ':', 3);
    Temp = getValue(strMessageFromCOM, ':', 4);
    Hum = getValue(strMessageFromCOM, ':', 5);
    Pres = getValue(strMessageFromCOM, ':', 6);

    //Serial.println(getValue(strMessageFromCOM, ':', 3));
    newData = false;
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "0";
}
//=======================================================================
