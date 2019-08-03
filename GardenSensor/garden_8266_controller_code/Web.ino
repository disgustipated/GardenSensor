//Webpage setup and config
void setupWeb(){
//  server.on("/device/levelstatus", HTTP_GET, getWaterLevel);
  server.on("/device/activate", HTTP_POST, activateDevice);
  server.on("/version", HTTP_GET, getVersion);
  server.on("/", handleRoot);
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  }

//web api calls
//main web service host
void handleRoot() { 
  digitalWrite(WIFI_INFO_LED_PIN, 1);
  char temp[1500];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  float sensorDataTemp = dht.readTemperature(true);
  float sensorDataHumd = dht.readHumidity();
  float sensorDataIndx = dht.computeHeatIndex(sensorDataTemp,sensorDataHumd);
  snprintf(temp, 1500,
           "<html>\
            <head>\
              <meta http-equiv='refresh' content='65'/>\
              <title>Garden ESP8266</title>\
              <style>\
                body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
              </style>\
              <script>\
                function activateDevice(){\
                    var xhr = new XMLHttpRequest();\
                    var url = \"http://6.13.0.219/device/activate\";\ 
                    xhr.\open(\"POST\",url, true);\  
                    xhr.\send();\
                  }\
              </script>\
            </head>\
            <body>\
              <h1>Garden ESP8266</h1>\
              <p>Uptime: %02d:%02d:%02d</p>\
              <p>Temp: %f</p>\
              <p>Humidity: %f</p>\
              <p>Heat Index: %f</p>\
              <button onclick=\"activateDevice()\">run pump</button>\
            </body>\
           </html>",
           hr, min % 60, sec % 60, sensorDataTemp, sensorDataHumd, sensorDataIndx
           );
  server.send(200, "text/html", temp);
  digitalWrite(WIFI_INFO_LED_PIN, 0);
}
void handleNotFound() {
  digitalWrite(WIFI_INFO_LED_PIN, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(WIFI_INFO_LED_PIN, 0);
}

void getVersion() {
  digitalWrite(WIFI_INFO_LED_PIN,HIGH);
  server.send(200,"text/plain", SOFTWARE_VERSION);
  digitalWrite(WIFI_INFO_LED_PIN,LOW);
}

void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}
