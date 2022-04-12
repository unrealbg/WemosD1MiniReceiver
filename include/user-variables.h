#include <Arduino.h>

#define Nextion Serial
const int relayPin = D5;
const int relayPin2 = D6;
const int relayPin3 = D7;
const int relayPin4 = D4;
const int Dry = 1024;
const int Wet = 1005;
const int SensorPin = A0;
int soilMoistureValue = 0;
float soilmoisturepercent = 0;
float Temperature;
float Humidity;
float lux;
float setTemp = 0;
float setLight = 0;
float setSoil = 0;
float setHumidity = 0;
bool btnTemp = false;
bool btnSoil = false;
bool btnHumidity = false;
bool btnLight = false;
bool isOff = true;
float RS_gas = 0;
float ratio = 0;
float sensorValue = 0;
float sensor_volt = 0;
float R0 = 6982.46;
void checkTemp();
void checkLight();
void checkSoilMoisture();
void sendPPM();
void sendLight();
void sendTempHumidity();
void sendUptime();
void callback(char *topic, byte *payload, unsigned int length);