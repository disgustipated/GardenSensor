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
