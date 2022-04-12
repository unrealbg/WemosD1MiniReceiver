/*
 Name:		Wemos d1 Mini Receiver
 Created:	5-Mar-22 13:22:30
 Author:	Zhelyazkov (unrealbg)
*/

#include <Arduino.h>
#include <mqtt-auth.h>
#include <user-variables.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <BH1750.h>
#include <SPI.h>
#include <Wire.h>
#include <Nextion.h>
#include <arduino-timer.h>
#include <uptime_formatter.h>
#include <AsyncElegantOTA.h>

NexText tState = NexText(0, 15, "tState");
NexText hState = NexText(0, 16, "hState");
NexText smState = NexText(0, 17, "smState");
NexText lState = NexText(0, 18, "lState");
NexButton b0on = NexButton(0, 8, "b0on");
NexButton b0off = NexButton(0, 9, "b0off");
NexButton b1on = NexButton(0, 4, "b1on");
NexButton b1off = NexButton(0, 5, "b1off");
NexButton b2on = NexButton(0, 2, "b2on");
NexButton b2off = NexButton(0, 3, "b2off");
NexButton b3on = NexButton(0, 6, "b3on");
NexButton b3off = NexButton(0, 7, "b3off");

NexTouch *nex_listen_list[] = {
    &b0on,
    &b0off,
    &b1on,
    &b1off,
    &b2on,
    &b2off,
    &b3on,
    &b3off,
    NULL};

WiFiClient espClient;
PubSubClient client(espClient);
BH1750 lightMeter(0x23);
DHTesp dht;
AsyncWebServer server(80);
DNSServer dns;

auto timer = timer_create_default();

void debugSentCallback(String data)
{
  Serial.println(data);
}

bool relayCheck(void *)
{
  checkTemp();
  checkLight();
  checkSoilMoisture();
  sendPPM();
  return true;
}

void b0onPushCallback(void *ptr)
{
  digitalWrite(relayPin, LOW);
  btnTemp = true;
  tState.setText("Включено");
  isOff = false;
  client.publish("home/wemos/buttonState/temp", "true");
}
void b0offPushCallback(void *ptr)
{
  digitalWrite(relayPin, HIGH);
  btnTemp = false;
  tState.setText("Изключено");
  isOff = true;
  client.publish("home/wemos/buttonState/temp", "false");
}
void b1onPushCallback(void *ptr)
{
  digitalWrite(relayPin4, LOW);
  btnHumidity = true;
  hState.setText("Включено");
  client.publish("home/wemos/buttonState/humidity", "true");
}
void b1offPushCallback(void *ptr)
{
  digitalWrite(relayPin4, HIGH);
  btnHumidity = false;
  hState.setText("Изключено");
  client.publish("home/wemos/buttonState/humidity", "false");
}
void b2onPushCallback(void *ptr)
{
  digitalWrite(relayPin2, LOW);
  btnSoil = true;
  smState.setText("Включено");
  client.publish("home/wemos/buttonState/soil", "true");
}
void b2offPushCallback(void *ptr)
{
  digitalWrite(relayPin2, HIGH);
  btnSoil = false;
  smState.setText("Изключено");
  client.publish("home/wemos/buttonState/soil", "false");
}
void b3onPushCallback(void *ptr)
{
  digitalWrite(relayPin3, LOW);
  btnLight = true;
  lState.setText("Включено");
  client.publish("home/wemos/buttonState/light", "true");
}
void b3offPushCallback(void *ptr)
{
  digitalWrite(relayPin3, HIGH);
  btnLight = false;
  lState.setText("Изключено");
  client.publish("home/wemos/buttonState/light", "false");
}

bool sendMsg(void *)
{
  sendLight();
  sendTempHumidity();
  return true;
}

void sendPPM()
{
  sensorValue = analogRead(A0);
  sensor_volt = sensorValue / 1024 * 5.0;
  RS_gas = (5.0 - sensor_volt) / sensor_volt;
  ratio = RS_gas / R0; // Replace R0 with the value found using the sketch above
  float x = 1538.46 * ratio;
  float ppm = pow(x, -1.709);
  ppm *= 2000;
  client.publish("home/wemos/ppm", String(ppm).c_str(), true);
}

void checkTemp()
{
  if (setTemp <= Temperature && setTemp != 0)
  {
    digitalWrite(relayPin, LOW);
    client.publish("home/wemos/buttonState/temp", "true");
    tState.setText("Включено");
    isOff = false;
  }
  else if (btnTemp != true && setTemp - 1 >= Temperature)
  {
    digitalWrite(relayPin, HIGH);
    tState.setText("Изключено");
    client.publish("home/wemos/buttonState/temp", "false");
    isOff = true;
  }

  if (setHumidity > Humidity && setHumidity != 0)
  {
    digitalWrite(relayPin4, LOW);
    client.publish("home/wemos/buttonState/humidity", "true");
    hState.setText("Включено");
  }
  else if (btnHumidity != true && setHumidity + 15 <= Humidity && setHumidity != 0)
  {
    digitalWrite(relayPin4, HIGH);
    client.publish("home/wemos/buttonState/humidity", "false");
    hState.setText("Изключено");
  }
}

void checkLight()
{
  if (lux <= setLight && setLight != 0)
  {
    digitalWrite(relayPin3, LOW);
    client.publish("home/wemos/buttonState/light", "true");
    lState.setText("Включено");
  }
  else if (btnLight != true && lux + 10 > setLight && setLight != 0)
  {
    digitalWrite(relayPin3, HIGH);
    client.publish("home/wemos/buttonState/light", "false");
    lState.setText("Изключено");
  }
}

void checkSoilMoisture()
{
  soilMoistureValue = analogRead(SensorPin);
  soilmoisturepercent = map(soilMoistureValue, Dry, Wet, 0, 100);

  if (soilmoisturepercent <= setSoil && setSoil)
  {
    digitalWrite(relayPin2, LOW);
    client.publish("home/wemos/buttonState/soil", "true");
    smState.setText("Включено");
  }
  else if (btnSoil != true && soilmoisturepercent > setSoil + 10 && setSoil != 0)
  {
    digitalWrite(relayPin2, HIGH);
    client.publish("home/wemos/buttonState/soil", "false");
    smState.setText("Изключено");
  }
}

void checkConnection()
{
  if (!client.connected())
  {
    Serial.println("MQTT Broker disconected!");
    Serial.println("Reconnect...");
    ESP.restart();
  }
}

void sendTempHumidity()
{
  String command = "temp.txt=\"" + String(Temperature, 1) + "\"";
  Serial.print(command);
  Serial.print("\xFF\xFF\xFF");

  String command2 = "humidity.txt=\"" + String(Humidity, 1) + "\"";
  Serial.print(command2);
  Serial.print("\xFF\xFF\xFF");
}

void sendLight()
{
  String command3 = "light.txt=\"" + String(lux, 1) + "\"";
  Serial.print(command3);
  Serial.print("\xFF\xFF\xFF");
}

void sendSoilMoisture()
{

  soilMoistureValue = analogRead(SensorPin);
  soilmoisturepercent = map(soilMoistureValue, Dry, Wet, 0, 100);

  float sto = 100;
  float nula = 0;

  if (soilmoisturepercent > 100)
  {
    String command4 = "soilMoisture.txt=\"" + String(sto, 1) + "\"";
    Serial.print(command4);
    Serial.print("\xFF\xFF\xFF");

    client.publish("home/wemos/soil", "100", true);
  }
  else if (soilmoisturepercent < 0)
  {
    String command5 = "soilMoisture.txt=\"" + String(nula, 1) + "\"";
    Serial.print(command5);
    Serial.print("\xFF\xFF\xFF");

    client.publish("home/wemos/soil", "0", true);
  }
  else if (soilmoisturepercent >= 0 && soilmoisturepercent <= 100)
  {
    String command6 = "soilMoisture.txt=\"" + String(soilmoisturepercent, 1) + "\"";
    Serial.print(command6);
    Serial.print("\xFF\xFF\xFF");

    client.publish("home/wemos/soil", String(soilmoisturepercent).c_str(), true);
  }
}

void setup_mqtt()
{
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");

    if (client.connect("WemosD1MiniReciver", mqttUser, mqttPassword))
    {
      Serial.println("connected");
    }
    else
    {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

bool sendUp(void *)
{
  sendUptime();
  return true;
}

void sendUptime()
{
  client.publish("home/wemos/uptime", String(uptime_formatter::getUptime()).c_str(), true);
}

void setup()
{
  // Debug console

  // Serial.begin(115200);
  Serial.begin(9600);
  nexInit();

  b0on.attachPush(b0onPushCallback, &b0on);
  b0off.attachPush(b0offPushCallback, &b0off);
  b1on.attachPush(b1onPushCallback, &b1on);
  b1off.attachPush(b1offPushCallback, &b1off);
  b2on.attachPush(b2onPushCallback, &b2on);
  b2off.attachPush(b2offPushCallback, &b2off);
  b3on.attachPush(b3onPushCallback, &b3on);
  b3off.attachPush(b3offPushCallback, &b3off);

  Wire.begin(D2, D1);
  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.autoConnect();
  setup_mqtt();

  pinMode(relayPin, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  digitalWrite(relayPin, HIGH);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, HIGH);
  digitalWrite(relayPin4, HIGH);
  timer.every(10000, sendMsg);
  timer.every(2000, relayCheck);
  timer.every(1000, sendUp);
  client.subscribe("#");
  AsyncElegantOTA.begin(&server);
  server.begin();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String message;

  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  String myTopic = String(topic);

  if (myTopic == "home/wemos/set/temp")
  {
    setTemp = message.toFloat();
  }
  else if (myTopic == "home/wemos/set/light")
  {
    setLight = message.toFloat();
  }
  else if (myTopic == "home/wemos/set/soil")
  {
    setSoil = message.toInt();
  }
  else if (myTopic == "home/wemos/set/humidity")
  {
    setHumidity = message.toInt();
  }
  else if (myTopic == "home/wemos/switch/tempRelay")
  {
    if (message == "true")
    {
      digitalWrite(relayPin, LOW);
      tState.setText("Включено");
      isOff = false;
    }
    else
    {
      digitalWrite(relayPin, HIGH);
      tState.setText("Изключено");
      isOff = true;
    }
  }
  else if (myTopic == "home/wemos/switch/lightRelay")
  {
    if (message == "true")
    {
      digitalWrite(relayPin3, LOW);
      lState.setText("Включено");
    }
    else
    {
      digitalWrite(relayPin3, HIGH);
      lState.setText("Изключено");
    }
  }
  else if (myTopic == "home/wemos/switch/humidityRelay")
  {
    if (message == "true")
    {
      digitalWrite(relayPin4, LOW);
      hState.setText("Включено");
    }
    else
    {
      digitalWrite(relayPin4, HIGH);
      hState.setText("Изключено");
    }
  }
  else if (myTopic == "home/wemos/switch/soilRelay")
  {
    if (message == "true")
    {
      digitalWrite(relayPin2, LOW);
      smState.setText("Включено");
    }
    else
    {
      digitalWrite(relayPin2, HIGH);
      smState.setText("Изключено");
    }
  }
  else if (myTopic == "home/wemos/temp")
  {
    Temperature = message.toFloat();
  }
  else if (myTopic == "home/wemos/humidity")
  {
    Humidity = message.toFloat();
  }
  else if (myTopic == "home/wemos/soil")
  {
    soilmoisturepercent = message.toFloat();
  }
  else if (myTopic == "home/wemos/light")
  {
    lux = message.toFloat();
  }
  else if (myTopic == "home/wemos/refresh/all" && message == "true")
  {
    String command = "temp.txt=\"" + String(Temperature, 1) + "\"";
    Serial.print(command);
    Serial.print("\xFF\xFF\xFF");

    String command2 = "humidity.txt=\"" + String(Humidity, 1) + "\"";
    Serial.print(command2);
    Serial.print("\xFF\xFF\xFF");

    client.publish("home/wemos/light", String(lux).c_str(), true);

    String command3 = "light.txt=\"" + String(lux, 1) + "\"";
    Serial.print(command3);
    Serial.print("\xFF\xFF\xFF");

    float sto = 100;
    float nula = 0;

    if (soilmoisturepercent > 100)
    {
      String command4 = "soilMoisture.txt=\"" + String(sto, 1) + "\"";
      Serial.print(command4);
      Serial.print("\xFF\xFF\xFF");
    }
    else if (soilmoisturepercent < 0)
    {
      String command5 = "soilMoisture.txt=\"" + String(nula, 1) + "\"";
      Serial.print(command5);
      Serial.print("\xFF\xFF\xFF");
    }
    else if (soilmoisturepercent >= 0 && soilmoisturepercent <= 100)
    {
      String command6 = "soilMoisture.txt=\"" + String(soilmoisturepercent, 1) + "\"";
      Serial.print(command6);
      Serial.print("\xFF\xFF\xFF");
    }

    client.publish("home/wemos/refresh/all", "false", true);
  }
}

void loop()
{
  client.loop();
  checkConnection();
  timer.tick();
  nexLoop(nex_listen_list);
}