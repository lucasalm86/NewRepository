/* This example shows how to bulk update a ThingSpeak channel using an Arduino MKR1000 or an ESP8266*/

#include<EthernetClient.h> //Uncomment this library to work with ESP8266
#include<ESP8266WiFi.h> //Uncomment this library to work with ESP8266
#include <Wire.h>
#include <SDS011.h>
float p10, p25;
#define pwmPin D8

SDS011 my_sds;

long th, tl, h, l, ppm;
int indDelaySecs, error;
int indDelayWork = 0;

#include<SPI.h> // Comment this to work with ESP8266 board
//#include<WiFi101.h> // Comment this to work with ESP8266 board

char jsonBuffer[500] = "["; // Initialize the jsonBuffer to hold data
char zapataja[] = ",";

char ssid[] = "Xia_n2"; //  Your network SSID (name)

char pass[] = "Envi_32Envi_32"; // Your network password
WiFiClient client; // Initialize the WiFi client library

char server[] = "api.thingspeak.com"; // ThingSpeak Server

String strMessageFromCOM, Co2, PM2_5, PM10, Temp, Hum, Pres, valueField, fieldVALUE;
boolean newData = false;

/* Collect data once every 15 seconds and post data to ThingSpeak channel once every 2 minutes */
unsigned long lastConnectionTime = 0; // Track the last connection time
unsigned long lastUpdateTime = 0; // Track the last update time
const unsigned long postingInterval = 120L * 1000L; // Post data every 2 minutes
const unsigned long updateInterval = 15L * 1000L; // Update once every 15 seconds

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Wire.begin();
  my_sds.begin(D6, D7);

  Serial.begin(115200);
  // Attempt to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
    delay(10000);  // Wait 10 seconds to connect
  }
  Serial.println("Connected to wifi");
  digitalWrite(LED_BUILTIN, HIGH);
  printWiFiStatus(); // Print WiFi connection information
}

void loop() {

  /**recvWithEndMarker();
    if (getValue(strMessageFromCOM, ':', 0) == "Arduino")
    {
    showNewData();
    strMessageFromCOM = "";
    }**/

  if (indDelayWork <= 200000)
  {
    indDelayWork++;
  } else
  {
    indDelaySecs++;
    indDelayWork = 0;
    //Serial.println("indDelayWork: " + String(indDelaySecs));
  }

  if (indDelaySecs == 600)
  {
    indDelaySecs = 0;
    error = my_sds.read(&p25, &p10);
    if (! error) {
      Serial.println("P2.5: " + String(p25));
      Serial.println("P10:  " + String(p10));
    } else
    {
      Serial.println("P2.5: error");
      p25 = 0;
      p10 = 0;
    }

    long tt = millis();

    //CO2 via pwm
    do
    {
      th = pulseIn(pwmPin, HIGH, 1004000) / 1000;
      tl = 1004 - th;
      //ppm2 = 2000 * (th-2)/(th+tl-4);
      ppm = 5000 * (th - 2) / (th + tl - 4);
      ppm = constrain(ppm, 350, 5000);
    } while (th == 0);
    Serial.println("Co2:" + String(ppm));


    // If update time has reached 15 seconds, then update the jsonBuffer

    updatesJson(jsonBuffer);
  }

}

// Updates the jsonBuffer with data
void updatesJson(char* jsonBuffer) {

  /* JSON format for updates paramter in the API
    This example uses the relative timestamp as it uses the "delta_t". If your device has a real-time clock, you can provide the absolute timestamp using the "created_at" parameter
    instead of "delta_t".
    "[{\"delta_t\":0,\"field1\":-70},{\"delta_t\":15,\"field1\":-66}]"
  */

  // Format the jsonBuffer as noted above
  strcat(jsonBuffer, "{\"delta_t\":");
  unsigned long deltaT = 0;
  size_t lengthT = String(deltaT).length();
  char temp[4];
  String(deltaT).toCharArray(temp, lengthT + 1);
  strcat(jsonBuffer, temp);
  strcat(jsonBuffer, ",");

  for (int i = 0; i <= 2; i++) {

    switch (i) {
      case 0:    // your hand is on the sensor
        valueField = "\"field1\":";
        fieldVALUE = String(ppm) + ",";
        break;
      case 1:    // your hand is close to the sensor
        valueField = "\"field2\":";
        fieldVALUE = String(p25) + ",";
        break;
      case 2:    // your hand is a few inches from the sensor
        valueField = "\"field3\":";
        fieldVALUE = String(p10);
        break;
    }
    //[{"delta_t":0,"field1":370},{"delta_t":0,"field2":0.00},{"delta_t":0,"field3":0.00}]}


    lengthT = valueField.length();
    valueField.toCharArray(temp, lengthT + 1);
    strcat(jsonBuffer, temp);

    lengthT = fieldVALUE.length();
    fieldVALUE.toCharArray(temp, lengthT + 1);
    strcat(jsonBuffer, temp);
  }
  strcat(jsonBuffer, "}]");


  // If posting interval time has reached 2 minutes, then update the ThingSpeak channel with your data
  //if (millis() - lastConnectionTime >=  postingInterval) {
  //size_t len = strlen(jsonBuffer);
 // jsonBuffer[len] = ']';

  //jsonBuffer[0] = '['; // Reinitialize the jsonBuffer for next batch of data
  //jsonBuffer[1] = '\0';
  httpRequest(jsonBuffer);
  //}
  //lastUpdateTime = millis(); // Update the last update time
}

// Updates the ThingSpeakchannel with data
void httpRequest(char* jsonBuffer) {
  /* JSON format for data buffer in the API
      This example uses the relative timestamp as it uses the "delta_t". If your device has a real-time clock, you can also provide the absolute timestamp using the "created_at" parameter
      instead of "delta_t".
       "{\"write_api_key\":\"YOUR-CHANNEL-WRITEAPIKEY\",\"updates\":[{\"delta_t\":0,\"field1\":-60},{\"delta_t\":15,\"field1\":200},{\"delta_t\":15,\"field1\":-66}]
  */
  // Format the data buffer as noted above
  char data[500] = "{\"write_api_key\":\"387JP60RMA4TDJO6\",\"updates\":"; //Replace YOUR-CHANNEL-WRITEAPIKEY with your ThingSpeak channel write API key
  strcat(data, jsonBuffer);
  strcat(data, "}");
  // Close any connection before sending a new request
  client.stop();
  String data_length = String(strlen(data) + 1); //Compute the data buffer length
  // POST data to ThingSpeak
  if (client.connect(server, 80)) {
    client.println("POST /channels/481473/bulk_update.json HTTP/1.1"); //Replace YOUR-CHANNEL-ID with your ThingSpeak channel ID
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + data_length);
    client.println();
    client.println(data);
    Serial.print("Sending request: " + String(data));
  }
  else {
    Serial.println("Failure: Failed to connect to ThingSpeak");
  }
  delay(250); //Wait to receive the response
  client.parseFloat();
  String resp = String(client.parseInt());
  Serial.println("Response code:" + resp); // Print the response code. 202 indicates that the server has accepted the response
  jsonBuffer[0] = '['; // Reinitialize the jsonBuffer for next batch of data
  jsonBuffer[1] = '\0';
  lastConnectionTime = millis(); // Update the last connection time
}

void printWiFiStatus() {
  // Print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print your device IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//=======================================================================

//---------------------------------------------------------------------
//recieving data functions
/**void recvWithEndMarker()
  {
  String str = "";

  if (Serial.available() > 0) {
    //Serial.println("Arduino getting string... " +  String(newData));
    strMessageFromCOM = Serial.readStringUntil('\n');
    delay(10);
    newData = true;
    Serial.print("Полученны данные с COM порта... отправляю на сервер...");
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
  }**/
//=======================================================================
