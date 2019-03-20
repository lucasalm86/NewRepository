/* This example shows how to bulk update a ThingSpeak channel using an Arduino MKR1000 or an ESP8266*/

#include<EthernetClient.h> //Uncomment this library to work with ESP8266
#include<ESP8266WiFi.h> //Uncomment this library to work with ESP8266
#include <ESP8266WebServer.h>
#include <EEPROM.h>

//объявление параметров для датчиков
//---------------------------------------------------------------------

#include <Wire.h>
#include <SDS011.h>
float p10, p25;
#define pwmPin D8 // пин для приема уровня СО2

SDS011 my_sds;
bool my_sds_onoff = false;

long th, tl, h, l, ppm;
int error;


ESP8266WebServer _server(80);
String ssidAP, passwordAP, content, ssid_login, ssid_password, passwordChanged, adminpasswordChanged, modeAP_RP_global, globalRP_IP, Co2_status, Co2_text_color, esid, epass;
String st, standartStrHtml, standartStrHtmlEnd;
bool clearEepromHttpAnswer, connectedToWiFiSsid, globalIP_status_Connection;
//примитивный подсчет времени
//---------------------------------------------------------------------
int indDelaySecs;
int indDelayWork = 0;
//=======================================================================

//#include<SPI.h> // Comment this to work with ESP8266 board

char jsonBuffer[500] = "["; // Initialize the jsonBuffer to hold data

char ssid[] = "Xia_n2"; //  Your network SSID (name)

char pass[] = "Envi_32Envi_32"; // Your network password
WiFiClient client; // Initialize the WiFi client library

char server[] = "api.thingspeak.com"; // ThingSpeak Server

String Co2, PM2_5, PM10, Temp, Hum, Pres, valueField, fieldVALUE;

/** Collect data once every 15 seconds and post data to ThingSpeak channel once every 2 minutes */
//unsigned long lastConnectionTime = 0; // Track the last connection time
//unsigned long lastUpdateTime = 0; // Track the last update time
//const unsigned long postingInterval = 120L * 1000L; // Post data every 2 minutes
//const unsigned long updateInterval = 15L * 1000L; // Update once every 15 seconds


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  EEPROM.begin(512);
  Serial.print("Writing EEPROM... ");
  //writeEEPROM("admin", "admin", "Xia_n2", "Envi_32Envi_32", "airDeviceTester", "Password", "AP");

  Wire.begin();
  my_sds.begin(D7, D6); // begin(uint8_t pin_rx, uint8_t pin_tx);

  Serial.begin(115200);


  if (EEPROM_ESP8266_READ(192, 224) == "RP")
  {
    esid = EEPROM_ESP8266_READ(64, 96);
    epass = EEPROM_ESP8266_READ(96, 128);

    Serial.println("Установлен режим RP");

    if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi(esid)) {
        Serial.println("Connected to wifi");
        digitalWrite(LED_BUILTIN, HIGH);
        printWiFiStatus(); // Print WiFi connection information
        launchWeb(0);
        return;
      }
    }
  }
  ssidAP = EEPROM_ESP8266_READ(128, 160);
  passwordAP = EEPROM_ESP8266_READ(160, 192);
  setupAP(true);

  /**
  // Attempt to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
    delay(10000);  // Wait 10 seconds to connect
  }
  **/
}

bool testWifi(String _ssid)
{
  int c = 0;
  Serial.println("Check for WiFi connected...");
  while ( c < 10 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi network - " + _ssid);
      return true;
    }
    delay(4000);
    //Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Подключение RP неустановлено.");
  return false;
}

void launchWeb(int webtype) {
  ////Serial.println("");
  ////Serial.println("WiFi connected");
  //Serial.print("Local IP: ");
  //Serial.println(WiFi.localIP());
  //Serial.print("SoftAP IP: ");

  //Serial.println(WiFi.softAPIP());
  //Serial.print("SoftAP SSID: ");
  //Serial.println(ssidAP);
  createWebServer(webtype);
  // Start the server
  _server.begin();
  //Serial.println("Server started");
}

void setupAP(bool sta) {
  if (sta)
  {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
  }
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    //Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li class=ssids onclick=myfunction(this)><a>";
    st += WiFi.SSID(i) + "</a>";
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssidAP.c_str(), passwordAP.c_str(), 6);
  Serial.println("softap");
  printWiFiStatus();
  connectedToWiFiSsid = true;
  launchWeb(1);
  //Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    _server.on("/", handleRoot);
    //clearEepromHttpAnswer = true; // выдать сообщение после очистки памяти EEPROM
    //server.on("/cleareeprom", clearEeprom);
    //server.on("/getIP_RPPage", getIP_RPPage);

    //server.on("/changeSsidPage", changeApSsid);

    //server.on("/setup", setupESP);
    //server.on("/adminAsk", adminAsk);
    //server.on("/logPassCorrect", logPassCorrect);
    //server.on("/ask_for_changeAP_RP_mode", ask_for_changeAP_RP_mode);
    //server.on("/save_changesAP_RP_mode_save", save_changesAP_RP_mode_save);
    //server.on("/changeAdminPage", ask_for_AdminPass_change);
    //server.on("/adminlogPassCorrect", adminlogPassCorrect);
  } else if (webtype == 0) {
    _server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      _server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
  }
}

void handleRoot()
{
  //Arduino:890:5:6:25:60:990
  Co2_status = "";
  if (Co2.toFloat() >= 601 && Co2.toFloat() <= 800) {
    Co2_status = "Yellow";
    Co2_text_color = "black";
  }
  if (Co2.toFloat() >= 801 && Co2.toFloat() <= 900) {
    Co2_status = "Orange";
    Co2_text_color = "black";
  }
  if (Co2.toFloat() >= 901 && Co2.toFloat() <= 1201) {
    Co2_status = "Coral";
    Co2_text_color = "black";
  }
  if (Co2.toFloat() >= 1201) {
    Co2_status = "DarkRed";
    Co2_text_color = "white";
  }
  Serial.print(Co2_status);
  String file1 =
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<style>.city{background-color:DarkGreen;color:white}.colorCo2{background-color:" + Co2_status + ";color:" + Co2_text_color + "}table{font-family:arial,sans-serif;border-collapse:collapse;width:60%}td,th{border:2px solid #ddd;text-align:center;padding:15px}tr:nth-child(even){background-color:#ddd}</style>\r\n"
    "<meta http-equiv=Content-type content=\"text/html; charset=utf-8\" />\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h2>Информация с датчиков Co2, PM2.5 и Температура-влажность-давление</h2>\r\n"
    "<table>\r\n"
    "<tr class=city>\r\n"
    "<th width=" + String("40%") + ">Датчик</th>\r\n"
    "<th width=" + String("70%") + ">Значение</th>\r\n"
    "</tr>\r\n"
    "<tr>\r\n"
    "<td>CO2</td>\r\n"
    "<td class='colorCo2'>" + Co2 + "</td>\r\n"
    "</tr>\r\n"
    "<tr>\r\n"
    "<td>PM2.5</td>\r\n"
    "<td class='value'>" + PM2_5 + "</td>\r\n"
    "</tr>\r\n"
    "<tr>\r\n"
    "<td>PM10</td>\r\n"
    "<td class='value'>" + PM10 + "</td>\r\n"
    "</tr>\r\n"
    "<tr>\r\n"
    "<td>Температура устройства</td>\r\n"
    "<td class='value'>" + Temp + "</td>\r\n"
    "</tr>\r\n"
    "<tr>\r\n"
    "<td>Влажность устройства</td>\r\n"
    "<td class='value'>" + Hum + "</td>\r\n"
    "</tr>\r\n"
    "<tr>\r\n"
    "<td>Атмосферное давление</td>\r\n"
    "<td class='value'>" + Pres + "</td>\r\n"
    "</tr>\r\n"
    "</table>\r\n"
    "</body>\r\n"
    "</html>\r\n";

  _server.send(200, "text/html", file1);
}

void handleNotFound()
{
  _server.sendHeader("Location", "/", true);
  _server.send(302, "text/plane", "File Not Find\n\n");
}

void loop() {

  if (indDelayWork <= 200000)
  {
    indDelayWork++;
  } else
  {
    indDelaySecs++;
    indDelayWork = 0;
    Serial.println("indDelayWork: " + String(indDelaySecs));
  }

  if (my_sds_onoff && indDelaySecs == 45)
  {
    my_sds.wakeup();
  }


  if (indDelaySecs == 60)
  {
    //showEEPROM();
    my_sds_onoff = true;
    indDelaySecs = 0;
    error = my_sds.read(&p25, &p10);
    if (! error) {
      Serial.println("P2.5: " + String(p25));
      Serial.println("P10:  " + String(p10));
      PM2_5 = p25;
      PM2_5 = p10;
    } else
    {
      Serial.println("P2.5: error");
      p25 = 0;
      p10 = 0;
    }

    my_sds.sleep();

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
    Co2 = ppm;


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

  for (int i = 0; i <= 2; i++) { // формирую длинный запрос

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


    lengthT = valueField.length(); // шаблон формируется в switch
    valueField.toCharArray(temp, lengthT + 1);
    strcat(jsonBuffer, temp);

    lengthT = fieldVALUE.length(); // шаблон формируется в switch
    fieldVALUE.toCharArray(temp, lengthT + 1);
    strcat(jsonBuffer, temp);
  }
  strcat(jsonBuffer, "}]");

  httpRequest(jsonBuffer); // отправляю запрос на сервак
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
  //  lastConnectionTime = millis(); // Update the last connection time
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


void showEEPROM()
{
  Serial.println("admin login: " + EEPROM_ESP8266_READ(0, 32));
  Serial.println("admin password: " + EEPROM_ESP8266_READ(32, 64));
  Serial.println("ssid login: " + EEPROM_ESP8266_READ(64, 96));
  Serial.println("ssid password: " + EEPROM_ESP8266_READ(96, 128));
  Serial.println("softAP ssid: " + EEPROM_ESP8266_READ(128, 160));
  Serial.println("softAP password: " + EEPROM_ESP8266_READ(160, 192));
  Serial.println("modeAP_RP: " + EEPROM_ESP8266_READ(192, 224));
}

String EEPROM_ESP8266_READ(int min, int max) {
  EEPROM.begin(512);
  delay(10);
  String buffer;
  for (int L = min; L < max; ++L)
    if (isGraph(EEPROM.read(L)))
      buffer += char(EEPROM.read(L));
  return buffer;
}

void writeEEPROM(String adminlogin, String adminpassword, String wifissid, String wifipassword, String softAPName, String softAPPassword, String modeAP_RP)
{
  if (adminlogin != "")
  {
    clearEepromESP8266(0, 32);
    EEPROM_ESP8266_WRITE(adminlogin, 0);
    //Serial.println("admin login changed to " + adminlogin);
  }

  if (adminpassword != "")
  {
    clearEepromESP8266(32, 64);
    EEPROM_ESP8266_WRITE(adminpassword, 32);
    //Serial.println("admin password changed to " + adminpassword);
  }

  if (wifissid != "")
  {
    clearEepromESP8266(64, 96);
    EEPROM_ESP8266_WRITE(wifissid, 64);
    //Serial.println("ssid login changed to " + wifissid);
  }

  if (wifipassword != "")
  {
    clearEepromESP8266(96, 128);
    EEPROM_ESP8266_WRITE(wifipassword, 96);
    //Serial.println("ssid password changed to " + wifipassword);
  }

  if (softAPName != "")
  {
    clearEepromESP8266(128, 160);
    EEPROM_ESP8266_WRITE(softAPName, 128);
    //Serial.println("softAP ssid changed to " + softAPName);
  }

  if (softAPPassword != "")
  {
    clearEepromESP8266(160, 192);
    EEPROM_ESP8266_WRITE(softAPPassword, 160);
    //Serial.println("softAP password changed to " + softAPPassword);
  }

  if (modeAP_RP != "")
  {
    clearEepromESP8266(192, 224);
    EEPROM_ESP8266_WRITE(modeAP_RP, 192);
    //Serial.println("modeAP_RP changed to " + modeAP_RP);
  }

  //Serial.println("admin login: " + EEPROM_ESP8266_READ(0, 32));
  //Serial.println("admin password: " + EEPROM_ESP8266_READ(32, 64));
  //Serial.println("ssid login: " + EEPROM_ESP8266_READ(64, 96));
  //Serial.println("ssid password: " + EEPROM_ESP8266_READ(96, 128));
  //Serial.println("softAP ssid: " + EEPROM_ESP8266_READ(128, 160));
  //Serial.println("softAP password: " + EEPROM_ESP8266_READ(160, 192));
  //Serial.println("modeAP_RP: " + EEPROM_ESP8266_READ(192, 224));
}

void clearEepromESP8266(int from, int to)
{
  Serial.println("clearing eeprom from " + String(from) + " to " + String(to));
  for (int i = from; i < to; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void EEPROM_ESP8266_WRITE(String buffer, int N) {
  EEPROM.begin(512);
  delay(10);
  for (int L = 0; L < 32; ++L) {
    EEPROM.write(N + L, buffer[L]);
  }
  EEPROM.commit();
}
