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
ESP8266WebServer server(8266); //this cant be running on port 80 because of the wifimanager
WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  pinMode(WIFI_RESET_PIN,INPUT_PULLUP);
  wifiManager.setSTAStaticIPConfig(IPAddress(6,13,0,218), IPAddress(6,13,0,1), IPAddress(255,255,255,0)); //Remove this for DHCP
  wifiManager.autoConnect("ESPSetup", "Setup1");
  client.setServer(mqtt_server, 1883);
  dht.begin();
  setupWeb();
}
void(* resetFunc) (void) = 0; //declare reset function @ address 0

//main loop
void loop() {
  // wire a button with a 10k resistor to ground and the other end to pin 14 for resetting and to prompt for new network
  if ( digitalRead(WIFI_RESET_PIN) == LOW ) {
    WiFiManager wifiManager;
    wifiManager.setTimeout(600);
    WiFi.mode(WIFI_STA);
    wifiManager.startConfigPortal("OnDemandAP");
    Serial.println("Autoconnect portal running");
  }
  //checking if mqtt is connected
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
