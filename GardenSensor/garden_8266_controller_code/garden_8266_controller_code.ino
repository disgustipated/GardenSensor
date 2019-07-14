#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define mqtt_server "6.13.0.144"
#define mqtt_user "garden"
#define mqtt_password "garden"
#define topic "home/garden"

IPAddress ip(6, 13, 0, 219); //Static IP address for this device.
IPAddress gw(6, 13, 0, 1); //Gateway address. IP address of your router
IPAddress sub(255, 255, 255, 0); //Subnet mask for this network

const String SOFTWARE_VERSION = "2.0 garden_esp8266_controller_code.ino";
const char* DEVICENAME = "gardensensor"; 
const int SENSOR_INFO_LED_PIN = 5;
const int WIFI_INFO_LED_PIN = 2;
const int WIFI_RESET_PIN = 14;
const int DEVICE_ACTIVATE_PIN = 4;
const long ACTIVATE_DURATION = 2000;
const long CHECK_WIFI_INTERVAL = 30000;
const long CHECK_MQTT_INTERVAL = 30000;
const long CHECK_SENSORS_INTERVAL = 5000;
unsigned long deviceActivateStart;
unsigned long prevMillisSensors = 0;
unsigned long prevMillisWiFi = 0;
unsigned long prevMillisMQTT = 0;
unsigned long currMillis = millis();

MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  pinMode(WIFI_RESET_PIN,INPUT);
  digitalWrite(WIFI_RESET_PIN, LOW);
  wifiManager.autoConnect("ESPSetup", "Setup1");
  client.setServer(mqtt_server, 1883);
  dht.begin();
}
void(* resetFunc) (void) = 0; //declare reset function @ address 0

//main loop
void loop() {
  Serial.println(WIFI_RESET_PIN);
  //if ( digitalRead(WIFI_RESET_PIN) == HIGH ) {
  //  WiFiManager wifiManager;
  //  wifiManager.startConfigPortal("OnDemandAP");
  //  Serial.println("Autoconnect portal running");
  //}
  //checkWiFi();
  checkMQTT();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  checkSensors();
  //need to add logic to handle multiple different devices in the server in the wifi.connect where the server.on are declared. need to add
  //sprinkler - this will be a relay that triggers the water relay thing connected to the inlet from the house water when the humidity is low - need to run some sprinkler things from the roof runners
  //soak - this will be connected to a water relay thinger thats run off of the rain barrels if they are above a certain level
  //manual water - this will handle watering if the rain barrels are empty
  server.handleClient();
  
}

//functions below
//checkwifi checks if wifi is connected and if not reconnects
void checkWiFi() {
  currMillis = millis();
  if (currMillis - prevMillisWiFi >= CHECK_WIFI_INTERVAL)
  {
    Serial.println("Checking WIFI");
    prevMillisWiFi = currMillis;
    if (WiFi.status() != WL_CONNECTED)
      connectToWiFi();
  }
}
//checks connection to mqtt and attempts connection if not connected
void checkMQTT() {
  currMillis = millis();
  if (currMillis - prevMillisMQTT >= CHECK_MQTT_INTERVAL)
  {
    Serial.println("Checking MQTT");
    prevMillisMQTT = currMillis;
    Serial.print("connected status: ");
    Serial.println(client.state());
    if (!client.connected()) {
      Serial.println("Not connected to MQTT, triggering reconnect");
      reconnect();
    }
    else
    {
      Serial.println("Client still connected");
    }
  }
}
//used in checkwifi() to init wifi connection
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gw, sub);
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Setting up MDNS responder");
  while (!mdns.begin(DEVICENAME)) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("MDNS responder started");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_INFO_LED_PIN, HIGH);
    delay(500);
    digitalWrite(WIFI_INFO_LED_PIN, LOW);
    delay(500);
  }
  digitalWrite(WIFI_INFO_LED_PIN, LOW);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  setupWeb();
}

void reconnect() {
  int reconnectCount = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    if (reconnectCount > 12){
      checkWiFi();
      }
    reconnectCount++;
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Garden", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void checkSensors(){
  delay(2000);
  StaticJsonDocument<50> mqttDoc;
  JsonObject mqttMsg = mqttDoc.to<JsonObject>();
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit and c
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  if (f != 2147483647 || h != 2147483647 || hif != 2147483647){
    mqttMsg["temp"] = f;
    mqttMsg["humidity"] = h;
    mqttMsg["indx"] = hif;
    serializeJson(mqttMsg, Serial);
    char buffer[512];
    size_t n = serializeJson(mqttMsg, buffer);
    client.publish(topic, buffer, n);
    } else {
      Serial.println(F("Invalid data from sensor"));
      mqttMsg["temp"] = 0;
      mqttMsg["humidity"] = 0;
      mqttMsg["indx"] = 0;
      char buffer[512];
      size_t n = serializeJson(mqttMsg, buffer);
      client.publish(topic, buffer, n);
      }
}

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
void getVersion() {
  digitalWrite(WIFI_INFO_LED_PIN,HIGH);
  server.send(200,"text/plain", SOFTWARE_VERSION);
  digitalWrite(WIFI_INFO_LED_PIN,LOW);
}
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

//web functions for devices
String getWaterLevel(int OpenSensor, int CloseSensor) {
  //old code repurposed from the garage door
  String state = "AJAR";
  Serial.println(digitalRead(OpenSensor));
  Serial.println(digitalRead(CloseSensor));
  if (digitalRead(OpenSensor) == LOW)
    state = "OPEN";
  else if (digitalRead(CloseSensor) == LOW)
    state = "CLOSED";

  Serial.print("Device Status = ");
  Serial.println(state);
return state;
}
void activateDevice()
{
  //this will need to be used to activate the pump on the rain barrel, manual water from hose, and sprinkler
  deviceActivateStart = millis();
  Serial.print("Activating deviceActivateStart = ");
  Serial.println(deviceActivateStart);   
  digitalWrite(DEVICE_ACTIVATE_PIN, HIGH);
  digitalWrite(WIFI_INFO_LED_PIN,HIGH);
  server.send(200,"text/plain", "OK"); 
  digitalWrite(WIFI_INFO_LED_PIN,LOW);
}
