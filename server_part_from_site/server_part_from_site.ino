/* This example shows how to bulk update a ThingSpeak channel using an Arduino MKR1000 or an ESP8266*/

#include<EthernetClient.h> //Uncomment this library to work with ESP8266
#include<ESP8266WiFi.h> //Uncomment this library to work with ESP8266
#include <ESP8266WebServer.h>

//объявление параметров для WEB-сервера
//---------------------------------------------------------------------
ESP8266WebServer server(80);
String ssidAP, passwordAP;
String st, standartStrHtml, standartStrHtmlEnd;
String content, ssid_login, ssid_password, passwordChanged, adminpasswordChanged, modeAP_RP_global, globalRP_IP;
int statusCode;
bool clearEepromHttpAnswer, connectedToWiFiSsid, globalIP_status_Connection;

//=====================================================================

//объявление параметров для EEPROM
//---------------------------------------------------------------------
#include <EEPROM.h>


//=====================================================================


//объявление параметров для датчиков
//---------------------------------------------------------------------

#include <Wire.h>
#include <SDS011.h>
float p10, p25;
#define pwmPin D8 // пин для приема уровня СО2

SDS011 my_sds;

long th, tl, h, l, ppm;
int error;
#define Addr 0x76



//примитивный подсчет времени
//---------------------------------------------------------------------
int indDelaySecs;
int indDelayWork = 0;
//=======================================================================

//#include<SPI.h> // Comment this to work with ESP8266 board

char jsonBuffer[500] = "["; // Initialize the jsonBuffer to hold data

char ssid[] = "D-Link_I"; //  Your network SSID (name)

char pass[] = "125896347gerda"; // Your network password
WiFiClient client; // Initialize the WiFi client library

char server_api[] = "api.thingspeak.com"; // ThingSpeak Server

String Co2, PM2_5, PM10, Temp, Hum, Pres, valueField, fieldVALUE, Co2_status, Co2_text_color, esid, epass;

const byte numChars = 64;
char receivedChars[numChars]; // an array to store the received data

void setup() {
  //Зажигаю встроенный LED на плате для индикации соединения с WiFi точкой доступа
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  //-------------

  //Настройка переменных касающихся перезаписи памяти и изменения пароля
  EEPROM.begin(512);
  passwordChanged = "0"; // переменная для индикации смены пароля - т.е. чтобы небыло смены пароля при обновлении браузера
  adminpasswordChanged = "0"; // переменная для индикации смены пароля Устройства - т.е. чтобы небыло смены пароля при обновлении браузера

  //modeAP_RP_global = EEPROM_ESP8266_READ(192, 224);

  showEEPROM();

  /** if (modeAP_RP_global == "RP")
    {
    esid = EEPROM_ESP8266_READ(64, 96);
    epass = EEPROM_ESP8266_READ(96, 128);

    Serial.println("Установлен режим RP");

    if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi(esid)) {
        Serial.println("Connected to wifi");
        launchWeb(0);
        return;
      }
    }
    }
    ssidAP = EEPROM_ESP8266_READ(128, 160);
    passwordAP = EEPROM_ESP8266_READ(160, 192);
    setupAP(true);


    // Настройка датчика пыли
    Wire.begin();
    my_sds.begin(D6, D7); // begin(uint8_t pin_rx, uint8_t pin_tx);

    Serial.begin(115200);
    /** // Attempt to connect to WiFi network
    while (WiFi.status() != WL_CONNECTED) {
     Serial.print("Attempting to connect to SSID: ");
     Serial.println(ssid);
     WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
     delay(10000);  // Wait 10 seconds to connect
    }**/

  /**
    digitalWrite(LED_BUILTIN, HIGH); // Индикация соединения с сетью WiFi
    printWiFiStatus(); // Print WiFi connection information **/
}

void loop() {

  if (indDelayWork <= 200000)
  {
    indDelayWork++;
  } else
  {
    indDelaySecs++;
    indDelayWork = 0;
    //Serial.println("indDelayWork: " + String(indDelaySecs));
  }

  if (indDelaySecs == 45)
  {
    my_sds.wakeup();
  }

  if (indDelaySecs == 60)
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


    // If update time has reached 15 seconds, then update the jsonBuffer

    //updatesJson(jsonBuffer);
  }
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
  if (client.connect(server_api, 80)) {
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

//Часть кода отвечающая за ответы с локального сервера и формирования WEB-страниц
//---------------------------------------------------------------------

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

  server.send(200, "text/html", file1);
}

void handleNotFound()
{
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plane", "File Not Find\n\n");
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
  server.begin();
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
  connectedToWiFiSsid = true;
  launchWeb(1);
  //Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", handleRoot);
    //clearEepromHttpAnswer = true; // выдать сообщение после очистки памяти EEPROM
    //server.on("/cleareeprom", clearEeprom);
    server.on("/getIP_RPPage", getIP_RPPage);

    server.on("/changeSsidPage", changeApSsid);

    server.on("/setup", setupESP);
    server.on("/adminAsk", adminAsk);
    server.on("/logPassCorrect", logPassCorrect);
    server.on("/ask_for_changeAP_RP_mode", ask_for_changeAP_RP_mode);
    server.on("/save_changesAP_RP_mode_save", save_changesAP_RP_mode_save);
    server.on("/changeAdminPage", ask_for_AdminPass_change);
    server.on("/adminlogPassCorrect", adminlogPassCorrect);
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
  }
}

void ask_for_AdminPass_change()
{
  String ask_for_AdminPass_change_html = standartStrHtml +
                                         "<form method=get action=adminlogPassCorrect enctype=text/plain>\r\n"
                                         "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Введите СУЩЕСТВУЮЩИЙ логин и пароль:</p>\r\n"
                                         "<input type=text id=Editbox2 style=display:block;width:30%;height:42px;z-index:0 name=login value spellcheck=false placeholder=\"Логин\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                                         "<input type=text id=Editbox1 style=display:block;width:30%;height:42px;z-index:1 name=password value spellcheck=false placeholder=\"Пароль\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                                         "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Введите НОВЫЙ логин и пароль:</p>\r\n"
                                         "<input type=text id=Editbox2 style=display:block;width:30%;height:42px;z-index:0 name=new_login value spellcheck=false placeholder=\"Новый логин\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                                         "<input type=text id=Editbox1 style=display:block;width:30%;height:42px;z-index:1 name=new_password value spellcheck=false placeholder=\"Новый пароль\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                                         "<br><input type=submit id=Send name value=Сохранить class=button>\r\n" + standartStrHtmlEnd;

  adminpasswordChanged = "1";

  server.send(200, "text/html", ask_for_AdminPass_change_html);
}

void adminlogPassCorrect()
{
  String device_login = server.arg("login");
  String device_password = server.arg("password");
  String temp_adminLogin = EEPROM_ESP8266_READ(0, 32);
  String temp_adminPassword = EEPROM_ESP8266_READ(32, 64);
  String new_login = server.arg("new_login");
  String new_password = server.arg("new_password");

  //Serial.println("Введен системный логин: " + device_login + "." + " Введен системный пароль: " + device_password + ".");
  //Serial.println("Системный логин в памяти: " + temp_adminLogin + "." + " Системный пароль в памяти: " + temp_adminPassword + ".");
  //Serial.println("Будет поменян ssid подключения: " + ssid_login + "." + " Будет поменян пароль подключения: " + ssid_password + ".");

  String pageContent;
  pageContent = "";

  if (device_login == temp_adminLogin && device_password == temp_adminPassword)
  {
    //Serial.println("проверяю разрешена ли запись: " + String(adminpasswordChanged));
    if (adminpasswordChanged == "0")
    {
      pageContent = standartStrHtml +
                    "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Перейдите в меню в настройки и повторите действия по смене логина и пароля.</p>\r\n" + standartStrHtmlEnd;

    } else if (adminpasswordChanged == "1")
    {
      pageContent = standartStrHtml +
                    "<br>\r\n"
                    "<br>\r\n"
                    "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Логин и пароль устройства успешно изменен!</p>\r\n" + standartStrHtmlEnd;
      adminpasswordChanged = "0";

      //writeEEPROM("admin", "admin", "D_link", "Password", "airDeviceTester", "Password", "AP");
      writeEEPROM((temp_adminLogin != new_login) ? new_login : "", (temp_adminPassword != new_password) ? new_password : "", "", "", "", "", "");
      server.send(200, "text/html", pageContent);
    }
  } else if (device_login != temp_adminLogin || device_password != temp_adminPassword)
  {
    //Serial.println("Неверное имя пользователя или пароль");
    pageContent = standartStrHtml +
                  "<br><br><p style=\"color:red;height:42px; font:18px Arial, Helvetica, sans-serif;\">Неверное имя пользователя или пароль. Перейдите в меню в настройки и повторите действия.</p>\r\n"
                  "<form method=get action=changeAdminPage enctype=text/plain>\r\n"
                  "<input type=submit id=Send name value=\"Вернуться назад\" class=button>\r\n"
                  "</form></body></html>\r\n";
  }
  //Serial.println(pageContent);
  server.send(200, "text/html", pageContent);
}

void ask_for_changeAP_RP_mode()
{
  String tempAP = "";
  String tempRP = "";
  (modeAP_RP_global == "AP") ? tempAP = "checked" : tempRP = "checked";

  String changeAP_RP_http = standartStrHtml +
                            "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Выберите необходимый режим работы устройства:</p>\r\n"
                            "<form action=\"/save_changesAP_RP_mode_save\">\r\n"
                            "<input type=\"radio\" name=\"gender\" value=\"AP\" " +  tempAP + "> Режим точки доступа<br>\r\n"
                            "<input type=\"radio\" name=\"gender\" value=\"RP\"" +  tempRP + "> Режим подключения к роутеру<br>\r\n"
                            "<br><br>\r\n<input type=submit value=Изменить class=button></form>\r\n" +
                            standartStrHtmlEnd;

  server.send(200, "text/html", changeAP_RP_http);
}

void save_changesAP_RP_mode_save()
{
  String modeStatus = server.arg("gender");
  String modeS;

  (modeStatus == "AP") ? modeAP_RP_global = "AP" : modeAP_RP_global = "RP";

  writeEEPROM("", "", "", "", "", "", modeStatus);

  (modeStatus == "AP") ? modeS = "Режим работы устройства установлен в режим точки доступа с именем точки доступа: " + EEPROM_ESP8266_READ(128, 160) : modeS = "Режим работы устройства установлен в режим подключения к роутеру с именем: " + EEPROM_ESP8266_READ(64, 96) + ". В настройках роутера установите статический IP для устройства.";

  String changeAP_RP_http = standartStrHtml +
                            "<p style='color:red;height:42px; font:18px Arial, Helvetica, sans-serif;'>" + modeS + "</p>\r\n"
                            "<br><br>\r\n"
                            "<p style='color:black;height:42px; font:18px Arial, Helvetica, sans-serif;'>Перехожу в установленный режим... Переподключитесь к выбранной сети.</p>\r\n" +
                            standartStrHtmlEnd;
  server.send(200, "text/html", changeAP_RP_http);
  delay(2000);

  if (modeStatus == "AP")
  {
    setupAP(true);
  } else
  {
    WiFi.begin(EEPROM_ESP8266_READ(64, 96).c_str(), EEPROM_ESP8266_READ(96, 128).c_str());
    if (testWifi(esid)) {
      launchWeb(0);
      return;
    }
    setupAP(true);
  }
}

void logPassCorrect()
{
  String device_login = server.arg("login");
  String device_password = server.arg("password");
  String temp_adminLogin = EEPROM_ESP8266_READ(0, 32);
  String temp_adminPassword = EEPROM_ESP8266_READ(32, 64);

  Serial.println("Введен системный логин: " + device_login + "." + " Введен системный пароль: " + device_password + ".");
  Serial.println("Системный логин в памяти: " + temp_adminLogin + "." + " Системный пароль в памяти: " + temp_adminPassword + ".");
  Serial.println("Будет поменян ssid подключения: " + ssid_login + "." + " Будет поменян пароль подключения: " + ssid_password + ".");

  String pageContent;
  pageContent = "";

  //writeEEPROM("admin", "admin", "D_link", "Password", "airDeviceTester", "Password");
  //Serial.println("admin login: " + EEPROM_ESP8266_READ(0, 32));
  //Serial.println("admin password: " + EEPROM_ESP8266_READ(32, 64));
  //Serial.println("ssid login: " + EEPROM_ESP8266_READ(64, 96));
  //Serial.println("ssid password: " + EEPROM_ESP8266_READ(96, 128));

  if (device_login == temp_adminLogin  && device_password == temp_adminPassword)
  {
    //Serial.println("проверяю разрешена ли запись: " + String(passwordChanged));
    if (passwordChanged == "0")
    {
      pageContent = standartStrHtml +
                    "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Для повторного изменения точки доступа перейдите в настройки.</p>\r\n" + standartStrHtmlEnd;

    } else if (passwordChanged == "1")
    {
      pageContent = standartStrHtml +
                    "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Точка доступа успешно изменена!</p>\r\n"
                    "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">Переподключаюсь к точке доступа...</p>\r\n"
                    "<p style=\"color:black;height:42px; font:18px Arial, Helvetica, sans-serif;\">При подключении к серверу через роутер, необходимо узнать IP выделенное роутером устройству или лучше прописать статический IP.</p>\r\n" + standartStrHtmlEnd;
      passwordChanged = "0";

      //Serial.println(device_login);
      //Serial.println("");
      //Serial.println(device_password);
      //Serial.println("");
      writeEEPROM("", "", ssid_login, ssid_password, "", "", "");
      server.send(200, "text/html", pageContent);
      delay(2000);
      WiFi.begin(ssid_login.c_str(), ssid_password.c_str());
      if (testWifi(esid)) {
        launchWeb(0);
        return;
      }
      setupAP(true);
    }
  } else if (device_login != temp_adminLogin || device_password != temp_adminPassword)
  {
    //Serial.println("Неверное имя пользователя или пароль");
    pageContent = standartStrHtml +
                  "<p style=\"color:red;height:42px; font:18px Arial, Helvetica, sans-serif;\">Неверное имя пользователя или пароль. Попробуйте еще раз или перейдите на главную</p>\r\n"
                  "<form method=get action=logPassCorrect enctype=text/plain>\r\n"
                  "<input type=text id=Editbox2 style=display:block;width:30%;height:42px;z-index:0 name=login value spellcheck=false placeholder=\"Логин\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                  "<input type=text id=Editbox1 style=display:block;width:30%;height:42px;z-index:1 name=password value spellcheck=false placeholder=\"Пароль\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                  "<input type=submit id=Send name value=Сохранить class=button></form>\r\n" +
                  standartStrHtmlEnd;
  }
  //Serial.println(pageContent);
  server.send(200, "text/html", pageContent);
}

void adminAsk()
{
  ssid_login = server.arg("ssid");
  ssid_password = server.arg("pass");

  String adminAsk = standartStrHtml +
                    "<p style=\"color:red;height:42px; font:18px Arial, Helvetica, sans-serif;\">Для смены точки доступа необходимо ввести пароль администратора устройства</p>\r\n"
                    "<form method=get action=logPassCorrect enctype=text/plain>\r\n"
                    "<input type=text id=Editbox2 style=display:block;width:30%;height:42px;z-index:0 name=login value spellcheck=false placeholder=\"Логин\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                    "<input type=text id=Editbox1 style=display:block;width:30%;height:42px;z-index:1 name=password value spellcheck=false placeholder=\"Пароль\" data--100-bottom=transform:translate(0%,0px); data-bottom-top=transform:translate(200%,0px);>\r\n"
                    "<input type=submit id=Send name value=Сохранить class=button></form>\r\n" +
                    standartStrHtmlEnd;

  passwordChanged = "1";

  server.send(200, "text/html", adminAsk);
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
  server.send(200, "text/html", content);
}

void setupESP()
{
  globalRP_IP = "Nan";
  globalIP_status_Connection = false;
  String modeString;
  (modeAP_RP_global == "RP" && testWifi(esid)) ? modeString = "<input class=button onClick='location.href=\"getIP_RPPage\"' method=\"get\" type=submit class=button2 margin-left=10 value=\"Получить IP >>\">\r\n" : modeString = "";

  String setupHtml = standartStrHtml +
                     "<div class=flex><div class>\r\n"
                     "<input onClick='location.href=\"changeSsidPage\"' method=\"get\" type=submit class=button value=\"Смена точки доступа WiFi\"><p><p/>\r\n"
                     "<input onClick='location.href=\"ask_for_changeAP_RP_mode\"' method=\"get\" type=submit class=button value=\"Смена режима WiFi устройства\" >\r\n" + modeString + "<p><p/>\r\n"
                     "<input onClick='location.href=\"changeAdminPage\"' method=\"get\" type=submit class=button value=\"Смена пароля администратора\"></div></div>\r\n" + standartStrHtmlEnd;
  passwordChanged = "0";

  //modeAP_RP_global

  server.send(200, "text/html", setupHtml);
}

void getIP_RPPage()
{
  String ipStringMessage;

  (globalRP_IP == "Nan") ? ipStringMessage = "<p style='color:black;height:42px; font:18px Arial, Helvetica, sans-serif;'>Соединение будет сброшено. Обновите страницу через 15 секунд (дождитесь точки доступа устройства).</p>\r\n" : ipStringMessage = "<p style='color:black;height:42px; font:18px Arial, Helvetica, sans-serif;'>Ваш IP на роутере:" + globalRP_IP + ". Переподключитесь к роутеру и введите этот IP. И лучше прописать статический IP.</p>\r\n";

  String getIP_RPPageHtml = standartStrHtml +
                            "<p style='color:black;height:42px; font:18px Arial, Helvetica, sans-serif;'>Соединение будет сброшено. Обновите страницу через 15 секунд (дождитесь точки доступа устройства).</p>\r\n" + standartStrHtmlEnd;

  server.send(200, "text/html", getIP_RPPageHtml);

  delay(2000);
  if (!globalIP_status_Connection)
  {
    if (globalRP_IP == "Nan")
    {
      String esid = EEPROM_ESP8266_READ(64, 96);
      String epass = EEPROM_ESP8266_READ(96, 128);

      if ( esid.length() > 1)
      {
        Serial.println("Отключаю подключения... Перехожу в режим AP");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        WiFi.softAP(ssidAP.c_str(), passwordAP.c_str(), 6);
        Serial.println("включен режим AP");
        if (testWifi(esid))
        {
          globalRP_IP = WiFi.localIP().toString();
          Serial.println("Получил IP - " + globalRP_IP);
          globalIP_status_Connection = true;
        }
      }
    } else
    {
      Serial.println("Возвращаю режим подключения на AP...");
      WiFi.disconnect();
      ssidAP = EEPROM_ESP8266_READ(128, 160);
      passwordAP = EEPROM_ESP8266_READ(160, 192);
      setupAP(true);
    }
  }
}

//=======================================================================



//Часть кода отвечающая за ответ датчика BME280
//---------------------------------------------------------------------

void getT_H_P_FromSensors()
{
  unsigned int b1[24];
  unsigned int data[8];
  unsigned int dig_H1 = 0;
  for (int i = 0; i < 24; i++)
  {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((136 + i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 24 bytes of data
    if (Wire.available() == 1)
    {
      b1[i] = Wire.read();
    }
  }

  // Convert the data
  // temp coefficients
  unsigned int dig_T1 = (b1[0] & 0xff) + ((b1[1] & 0xff) * 256);
  int dig_T2 = b1[2] + (b1[3] * 256);
  int dig_T3 = b1[4] + (b1[5] * 256);

  // pressure coefficients
  unsigned int dig_P1 = (b1[6] & 0xff) + ((b1[7] & 0xff ) * 256);
  int dig_P2 = b1[8] + (b1[9] * 256);
  int dig_P3 = b1[10] + (b1[11] * 256);
  int dig_P4 = b1[12] + (b1[13] * 256);
  int dig_P5 = b1[14] + (b1[15] * 256);
  int dig_P6 = b1[16] + (b1[17] * 256);
  int dig_P7 = b1[18] + (b1[19] * 256);
  int dig_P8 = b1[20] + (b1[21] * 256);
  int dig_P9 = b1[22] + (b1[23] * 256);

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select data register
  Wire.write(161);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Request 1 byte of data
  Wire.requestFrom(Addr, 1);

  // Read 1 byte of data
  if (Wire.available() == 1)
  {
    dig_H1 = Wire.read();
  }

  for (int i = 0; i < 7; i++)
  {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((225 + i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 7 bytes of data
    if (Wire.available() == 1)
    {
      b1[i] = Wire.read();
    }
  }

  // Convert the data
  // humidity coefficients
  int dig_H2 = b1[0] + (b1[1] * 256);
  unsigned int dig_H3 = b1[2] & 0xFF ;
  int dig_H4 = (b1[3] * 16) + (b1[4] & 0xF);
  int dig_H5 = (b1[4] / 16) + (b1[5] * 16);
  int dig_H6 = b1[6];

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select control humidity register
  Wire.write(0xF2);
  // Humidity over sampling rate = 1
  Wire.write(0x01);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select control measurement register
  Wire.write(0xF4);
  // Normal mode, temp and pressure over sampling rate = 1
  Wire.write(0x27);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select config register
  Wire.write(0xF5);
  // Stand_by time = 1000ms
  Wire.write(0xA0);
  // Stop I2C Transmission
  Wire.endTransmission();

  for (int i = 0; i < 8; i++)
  {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((247 + i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 8 bytes of data
    if (Wire.available() == 1)
    {
      data[i] = Wire.read();
    }
  }

  // Convert pressure and temperature data to 19-bits
  long adc_p = (((long)(data[0] & 0xFF) * 65536) + ((long)(data[1] & 0xFF) * 256) + (long)(data[2] & 0xF0)) / 16;
  long adc_t = (((long)(data[3] & 0xFF) * 65536) + ((long)(data[4] & 0xFF) * 256) + (long)(data[5] & 0xF0)) / 16;
  // Convert the humidity data
  long adc_h = ((long)(data[6] & 0xFF) * 256 + (long)(data[7] & 0xFF));

  // Temperature offset calculations
  double var1 = (((double)adc_t) / 16384.0 - ((double)dig_T1) / 1024.0) * ((double)dig_T2);
  double var2 = ((((double)adc_t) / 131072.0 - ((double)dig_T1) / 8192.0) *
                 (((double)adc_t) / 131072.0 - ((double)dig_T1) / 8192.0)) * ((double)dig_T3);
  double t_fine = (long)(var1 + var2);
  double cTemp = (var1 + var2) / 5120.0;
  double fTemp = cTemp * 1.8 + 32;

  // Pressure offset calculations
  var1 = ((double)t_fine / 2.0) - 64000.0;
  var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
  var2 = var2 + var1 * ((double)dig_P5) * 2.0;
  var2 = (var2 / 4.0) + (((double)dig_P4) * 65536.0);
  var1 = (((double) dig_P3) * var1 * var1 / 524288.0 + ((double) dig_P2) * var1) / 524288.0;
  var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
  double p = 1048576.0 - (double)adc_p;
  p = (p - (var2 / 4096.0)) * 6250.0 / var1;
  var1 = ((double) dig_P9) * p * p / 2147483648.0;
  var2 = p * ((double) dig_P8) / 32768.0;
  double pressure = (p + (var1 + var2 + ((double)dig_P7)) / 16.0) / 100;

  // Humidity offset calculations
  double var_H = (((double)t_fine) - 76800.0);
  var_H = (adc_h - (dig_H4 * 64.0 + dig_H5 / 16384.0 * var_H)) * (dig_H2 / 65536.0 * (1.0 + dig_H6 / 67108864.0 * var_H * (1.0 + dig_H3 / 67108864.0 * var_H)));
  double humidity = var_H * (1.0 -  dig_H1 * var_H / 524288.0);
  if (humidity > 100.0)
  {
    humidity = 100.0;
  }
  else if (humidity < 0.0)
  {
    humidity = 0.0;
  }

  // Output data to serial monitor

  /** display.setCursor(0, 24);
    Serial.println("Temp:  " + String(cTemp) + " C");
    display.print("Temp:" + String(cTemp)  + " C");

    display.setCursor(0, 32);
    Serial.println("Press:  " + String(pressure)  + " hPa");
    display.print("Press:" + String(pressure) + "");

    display.setCursor(0, 40);
    Serial.println("Hum:  " + String(humidity)  + " %");
    display.print("Hum:" + String(humidity) + " %");

    /*Serial.print("Temperature in Celsius : ");
     Serial.print(cTemp);
     Serial.println(" C");
     Serial.print("Temperature in Fahrenheit : ");
     Serial.print(fTemp);
     Serial.println(" F");
     Serial.print("Pressure : ");
     Serial.print(pressure);
     Serial.println(" hPa");
     Serial.print("Relative Humidity : ");
     Serial.print(humidity);
     Serial.println(" RH");
     delay(1000);*/
}
//=======================================================================

//Функция с повторяющейся трокой
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
//-----------------------------

//Функции записи/перезаписи постоянной памяти
//=======================================================================
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
}

void EEPROM_ESP8266_WRITE(String buffer, int N) {
  EEPROM.begin(512);
  delay(10);
  for (int L = 0; L < 32; ++L) {
    EEPROM.write(N + L, buffer[L]);
  }
  EEPROM.commit();
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

void clearEepromESP8266(int from, int to)
{
  Serial.println("clearing eeprom from " + String(from) + " to " + String(to));
  for (int i = from; i < to; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

//-----------------------------
