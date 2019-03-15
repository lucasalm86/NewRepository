#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

String ssidAP, passwordAP;
String st, standartStrHtml, standartStrHtmlEnd;
String content, ssid_login, ssid_password, passwordChanged, adminpasswordChanged, modeAP_RP_global, globalRP_IP;
int statusCode;
bool clearEepromHttpAnswer, connectedToWiFiSsid, globalIP_status_Connection;

const byte numChars = 64;
char receivedChars[numChars]; // an array to store the received data
String strMessageFromCOM, Co2 = "разогрев", Co2_status, Co2_text_color, PM2_5 = "разогрев", PM10 = "разогрев", Temp = "разогрев", Hum = "разогрев", Pres = "разогрев";
int inc;

boolean newData = false;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  delay(10);
  //Serial.println();
  //Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  passwordChanged = "0"; // переменная для индикации смены пароля - т.е. чтобы небыло смены пароля при обновлении браузера
  adminpasswordChanged = "0"; // переменная для индикации смены пароля Устройства - т.е. чтобы небыло смены пароля при обновлении браузера
  //Serial.println("Reading EEPROM ssid");
  standartStr();

  modeAP_RP_global = EEPROM_ESP8266_READ(192, 224);

  showEEPROM();

  if (modeAP_RP_global == "RP")
  {
    String esid = EEPROM_ESP8266_READ(64, 96);
    String epass = EEPROM_ESP8266_READ(96, 128);

    Serial.println("Установлен режим RP");

    if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      }
    }
  }
  ssidAP = EEPROM_ESP8266_READ(128, 160);
  passwordAP = EEPROM_ESP8266_READ(160, 192);
  setupAP(true);

  //clearEepromHttpAnswer = false;
  //clearEeprom();
  //delay(200);
  //writeEEPROM("admin", "admin", "D_link", "Password", "airDeviceTester", "Password", "AP");
  //writeEEPROM("", "", "", "", "", "", "AP");
  //delay(200);
  //showEEPROM();
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
      if (testWifi()) {
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
    if (testWifi()) {
      launchWeb(0);
      return;
    }
    setupAP(true);
  }
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
        if (testWifi())
        {
          globalRP_IP = String(WiFi.localIP());
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

void setupESP()
{
  globalRP_IP = "Nan";
  globalIP_status_Connection = false;
  String modeString;
  (modeAP_RP_global == "RP" && testWifi()) ? modeString = "<input class=button onClick='location.href=\"getIP_RPPage\"' method=\"get\" type=submit class=button2 margin-left=10 value=\"Получить IP >>\">\r\n" : modeString = "";

  String setupHtml = standartStrHtml +
                     "<div class=flex><div class>\r\n"
                     "<input onClick='location.href=\"changeSsidPage\"' method=\"get\" type=submit class=button value=\"Смена точки доступа WiFi\"><p><p/>\r\n"
                     "<input onClick='location.href=\"ask_for_changeAP_RP_mode\"' method=\"get\" type=submit class=button value=\"Смена режима WiFi устройства\" >\r\n" + modeString + "<p><p/>\r\n"
                     "<input onClick='location.href=\"changeAdminPage\"' method=\"get\" type=submit class=button value=\"Смена пароля администратора\"></div></div>\r\n" + standartStrHtmlEnd;
  passwordChanged = "0";

  //modeAP_RP_global

  server.send(200, "text/html", setupHtml);
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

void clearEepromESP8266(int from, int to)
{
  Serial.println("clearing eeprom from " + String(from) + " to " + String(to));
  for (int i = from; i < to; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void handleRoot()
{
  passwordChanged = "0";
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
  //Serial.print(Co2_status);
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

  server.send(200, "text/html", file1);
}

void loop() {
  if (!connectedToWiFiSsid && !testWifi()) {
    setupAP(true);
  }
  server.handleClient();
  recvWithEndMarker();
  //showNewData();
  if (getValue(strMessageFromCOM, ':', 0) == "Arduino")
  {
    showNewData();
    //showNewData();
    strMessageFromCOM = "";
  }

  if (getValue(strMessageFromCOM, ':', 0) == "Sysinfo")
  {
    showEEPROM();
    strMessageFromCOM = "";
  }
  delay(50);
}

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
