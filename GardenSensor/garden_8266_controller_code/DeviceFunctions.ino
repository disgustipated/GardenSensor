void checkSensors(){
  currMillis = millis();
  if (currMillis - prevMillisSensors >= CHECK_SENSORS_INTERVAL){
  StaticJsonDocument<50> mqttDoc;
  prevMillisSensors = currMillis;
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
    //serializeJsonPretty(mqttMsg, Serial);
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
}

//web functions for devices
String getWaterLevel(int OpenSensor, int CloseSensor) {
  //old code repurposed from the garage door, not implemented yet
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

void pumpRunning()
{
  //should the pump be running
  currMillis = millis();
  if ((digitalRead(PUMP_ACTIVATE_PIN) == LOW) && (currMillis > (deviceActivateStart + ACTIVATE_DURATION))){
    stopPump();
    }
}

void activatePump()
{
  Alarm.delay(500);
  deviceActivateStart = millis();
  server.send(200,"text/plain", "OK"); 
  Serial.print("Activating deviceActivateStart = ");
  Serial.println(deviceActivateStart);   
  Serial.println(deviceActivateStart + ACTIVATE_DURATION);
  digitalWrite(PUMP_ACTIVATE_PIN, LOW);
  digitalWrite(WIFI_INFO_LED_PIN,LOW);
}

void stopPump()
{
  Serial.println("Stopping pump");
  digitalWrite(PUMP_ACTIVATE_PIN, HIGH);
  digitalWrite(WIFI_INFO_LED_PIN,HIGH);
}

void getTimeFromNtp() {
  // Begin NTP
  Serial.print("Begin NTP...");
  timeClient.begin();
  while (!timeClient.update()) yield();
  timeStruct.hours = timeClient.getHours();
  timeStruct.minutes = timeClient.getMinutes();
  timeStruct.seconds = timeClient.getSeconds();
  timeStruct.nextNtp = ntpInterval;
  Serial.println(" OK.");
}
