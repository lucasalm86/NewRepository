/* This example shows how to bulk update a ThingSpeak channel using an Arduino MKR1000 or an ESP8266*/

#include<EthernetClient.h> //Uncomment this library to work with ESP8266
#include<ESP8266WiFi.h> //Uncomment this library to work with ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
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
int wifiConnectingTry = 0;

//примитивный подсчет времени
//---------------------------------------------------------------------
int indDelaySecs;
int indDelayWork = 0;
//=======================================================================

//#include<SPI.h> // Comment this to work with ESP8266 board

char jsonBuffer[500] = "["; // Initialize the jsonBuffer to hold data

//char ssid[] = "Xia_n2"; //  Your network SSID (name)

//char pass[] = "Envi_32Envi_32"; // Your network password
WiFiClient client; // Initialize the WiFi client library
IPAddress myIP = WiFi.softAPIP();

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
  //writeEEPROM("admin", "admin", "Xia_n2", "Envi_32Envi_32", "airDeviceTester", "123456789", "Station"); //AP

  Wire.begin();
  my_sds.begin(D7, D6); // begin(uint8_t pin_rx, uint8_t pin_tx);

  Serial.begin(115200);

  standartStr();
  modeAP_RP_global = "AP";//EEPROM_ESP8266_READ(192, 224);


  if (modeAP_RP_global == "Station")
  {
    esid = EEPROM_ESP8266_READ(64, 96);
    epass = EEPROM_ESP8266_READ(96, 128);

    Serial.println("Установлен режим Station...");

    // Attempt to connect to WiFi network
    while (WiFi.status() != WL_CONNECTED || wifiConnectingTry == 10) {
      wifiConnectingTry++;
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(esid);
      WiFi.mode(WIFI_STA);
      WiFi.begin(esid.c_str(), epass.c_str());  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      delay(10000);  // Wait 10 seconds to connect
    }
    digitalWrite(LED_BUILTIN, HIGH);

    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }

    _server.on("/", handleRoot);

    _server.begin();
    Serial.println("HTTP server started");

    //launchWeb(1);
  }
  if (modeAP_RP_global == "AP" || wifiConnectingTry  == 10)
  {
    wifiConnectingTry = 0;
    ssidAP = EEPROM_ESP8266_READ(128, 160);
    //passwordAP = EEPROM_ESP8266_READ(160, 192);
    Serial.print("Creating Station with name-SSID: ");
    Serial.println(ssidAP);

    WiFi.softAP(ssidAP.c_str());
    Serial.println("softap");
    printWiFiStatus();
    connectedToWiFiSsid = true;
    digitalWrite(LED_BUILTIN, HIGH);

    
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    _server.on("/", handleRoot);
    _server.on("/changeSsidPage", changeApSsid);
    _server.begin();
    Serial.println("HTTP server started");
    //launchWeb(1);
  }
}

void loop() {

  _server.handleClient();

  if (indDelayWork <= 200000)
  {
    indDelayWork++;
  } else
  {
    indDelaySecs++;
    indDelayWork = 0;
    Serial.println("indDelayWork: " + String(indDelaySecs));
  }

  if (my_sds_onoff && indDelaySecs == 2)
  {
    my_sds.wakeup();
  }


  if (indDelaySecs == 6)
  {
    //showEEPROM();
    my_sds_onoff = true;
    indDelaySecs = 0;
    error = my_sds.read(&p25, &p10);
    if (! error) {
      Serial.println("P2.5: " + String(p25));
      Serial.println("P10:  " + String(p10));
      PM2_5 = p25;
      PM10 = p10;
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

void launchWeb(int webtype) {
  ////Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  //Serial.print("SoftAP IP: ");

  //Serial.println(WiFi.softAPIP());
  //Serial.print("SoftAP SSID: ");
  //Serial.println(ssidAP);
  createWebServer(webtype);
  // Start the server
  _server.begin();
  Serial.println("Server started");
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
  /**WiFi.softAP(ssidAP.c_str(), passwordAP.c_str(), 6);
    Serial.println("softap");
    printWiFiStatus();
    connectedToWiFiSsid = true;
    launchWeb(1);**/
  //Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    _server.on("/", handleRoot);
    _server.onNotFound(handleNotFound);
    //clearEepromHttpAnswer = true; // выдать сообщение после очистки памяти EEPROM
    //server.on("/cleareeprom", clearEeprom);
    //server.on("/getIP_RPPage", getIP_RPPage);

    _server.on("/changeSsidPage", changeApSsid);

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

void standartStr()
{
  standartStrHtml =
    "<!doctype html><html><head>\r\n"
    "<style>.ssids{color:black;padding:5px;border-radius:0.5rem;background-color:#C0C0C0;line-height:1.4rem;font-size:1.2rem;width:25%;}\r\n"
    ".button{text-decoration:none;text-align:center;padding:11px 32px;border:solid 1px #004f72;-webkit-border-radius:4px;-moz-border-radius:4px;border-radius:4px;font:18px Arial,Helvetica,sans-serif;font-weight:bold;color:#e5ffff;background-color:#3ba4c7;background-image:-moz-linear-gradient(top,#3BA4C7 0,#1982A5 100%);background-image:-webkit-linear-gradient(top,#3BA4C7 0,#1982A5 100%);background-image:-o-linear-gradient(top,#3BA4C7 0,#1982A5 100%);background-image:-ms-linear-gradient(top,#3BA4C7 0,#1982A5 100%);filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#1982A5',endColorstr='#1982A5',GradientType=0);background-image:linear-gradient(top,#3BA4C7 0,#1982A5 100%);-webkit-box-shadow:0 0 2px #bababa,inset 0 0 1px #fff;-moz-box-shadow:0 0 2px #bababa,inset 0 0 1px #fff;box-shadow:0 0 2px #bababa,inset 0 0 1px #fffwidth:40%;}\r\n"
    ".button2{color:white;padding:10px;border:0;border-radius:0.5rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:30%;}</style>\r\n"
    "<meta http-equiv=Content-type content=\"text/html;charset=utf-8\"/>\r\n"
    "<title>AirDeviceTester page</title></head><body><a href=\"/\">Главная</a><a class=button2 href=\"/setup\" method=\"get\">Настройки</a><br><br>\r\n";

  standartStrHtmlEnd =
    "</form></body></html>\r\n";
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Check for WiFi connected...");
  while ( c < 10 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi network - " + EEPROM_ESP8266_READ(64, 96));
      return true;
    }
    delay(1000);
    //Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Подключение RP неустановлено.");
  return false;
}

void setupESP()
{
  globalRP_IP = "Nan";
  globalIP_status_Connection = false;
  String modeString;
  (modeAP_RP_global == "Station" && testWifi()) ? modeString = "<input class=button onClick='location.href=\"getIP_RPPage\"' method=\"get\" type=submit class=button2 margin-left=10 value=\"Получить IP >>\">\r\n" : modeString = "";

  String setupHtml = standartStrHtml +
                     "<div class=flex><div class>\r\n"
                     "<input onClick='location.href=\"changeSsidPage\"' method=\"get\" type=submit class=button value=\"Смена точки доступа WiFi\"><p><p/>\r\n"
                     "<input onClick='location.href=\"ask_for_changeAP_RP_mode\"' method=\"get\" type=submit class=button value=\"Смена режима WiFi устройства\" >\r\n" + modeString + "<p><p/>\r\n"
                     "<input onClick='location.href=\"changeAdminPage\"' method=\"get\" type=submit class=button value=\"Смена пароля администратора\"></div></div>\r\n" + standartStrHtmlEnd;
  passwordChanged = "0";

  //modeAP_RP_global

  _server.send(200, "text/html", setupHtml);
}

void changeApSsid()
{
  setupAP(false);

  String settingsHtml = standartStrHtml +
                        "<h2>Выберите точку доступа из списка (просто кликните по ней):</h2>\r\n";

  content = "";

  //IPAddress ip = WiFi.softAPIP();
  //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  content += settingsHtml;
  content += "<p>";
  content += st;
  content += "</p><form method='get' action='adminAsk'><input id='ssid' name='ssid' length=32 placeholder='Имя точки доступа'><input name='pass' length=64 placeholder='Пароль'><input type='submit'></form>";
  content += "<script> function myfunction(ctrl){document.getElementById(\"ssid\").value = ctrl.getElementsByTagName('a')[0].innerHTML;}</script>";
  content += "</body></html>";
  _server.send(200, "text/html", content);
}

void handleRoot()
{
  Serial.print("Показываю окно в браузере...");
  
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
    /**
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
    "</html>\r\n";**/

    String file1 = "<!DOCTYPE html><html><head>\r\n"
                 "<style>.city{background-color:DarkGreen;color:white}.colorCo2{background-color:" +
                 Co2_status + ";color:" + Co2_text_color + "}.button{color:white;padding:10px;border:0;border-radius:0.5rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:30%;}table{font-family:arial,sans-serif;border-collapse:collapse;width:60%}td,th{border:2px solid #ddd;text-align:center;padding:15px}tr:nth-child(even){background-color:#ddd}</style>\r\n"
                 "<meta http-equiv=Content-type content=\"text/html; charset=utf-8\" />\r\n"
                 "</head><body><a class=button>Главная</a>\r\n"
                 "<a href=\"/setup\" method=\"get\">Настройки</a>\r\n"
                 "<h2>Информация с датчиков Co2, PM2.5 и Температура-влажность-давление</h2>\r\n"
                 "<table><tr class=city>\r\n"
                 "<th width=" + String("40%") + ">Датчик</th>\r\n"
                 "<th width=" + String("70%") + ">Значение</th>\r\n"
                 "</tr><tr><td>CO2</td><td class='colorCo2'>" + Co2 + "</td>\r\n"
                 "</tr><tr><td>PM2.5</td><td class='value'>" + PM2_5 + "</td>\r\n"
                 "</tr><tr><td>PM10</td><td class='value'>" + PM10 + "</td>\r\n"
                 "</tr><tr><td>Температура устройства</td><td class='value'>" + Temp + "</td>\r\n"
                 "</tr><tr><td>Влажность устройства</td><td class='value'>" + Hum + "</td>\r\n"
                 "</tr><tr><td>Атмосферное давление</td><td class='value'>" + Pres + "</td>\r\n"
                 "</tr></table>\r\n" + standartStrHtmlEnd;

    _server.send(200, "text/html", file1);
}

void handleNotFound()
{
  _server.sendHeader("Location", "/", true);
  _server.send(302, "text/plane", "File Not Find\n\n");
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
