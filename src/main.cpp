#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>

#include "Config.h"
String apiKey = API_KEY;
const char password[] = NET_PASS;
const char ssid[] = SSID;
const char server[] = HOST_NAME;

#define SEALEVELPRESSURE_HPA (1013.25)
#define DELAY_TIME (60000)//seconds
#define SLEEPTIME (6e7)//microseconds

float convertF (float);
void sendDataAndPrintValues (float, float, float);
void printValues (float, float, float);
void printWiFiStatus (WiFiClient);

Adafruit_BME280 bme; // I2C
WiFiClient client;

double tempF         = 0;
double pressure      = 0;
double humidity      = 0;
bool bmeStatus       = 0;

void setup() {

  Serial.begin(9600);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //
  bmeStatus = bme.begin();
  if (!bmeStatus) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
    
    ESP.deepSleep(SLEEPTIME);
  }

  // TODO: send up a "down" status to the channel metadata
  // if the bme status was bad
  // else send up status "up"

  // TODO: make request for channel meta location
  // Make secon request for live barometric pressure
  // Use it to set the live SEALEVELPRESSURE_HPA for the day 
  // instead of using default value
  // take some readings
  tempF = convertF(bme.readTemperature());
  pressure = (bme.readPressure() / 1000.0F); //kPa
  humidity = bme.readHumidity();

  // post them up
  sendDataAndPrintValues(tempF, humidity, pressure);

  ESP.deepSleep(SLEEPTIME);
}

void loop() {

}

void sendDataAndPrintValues (float tempF, float humid, float pressure) {
  client.connect(server, 80);
  if (client.connected()) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(tempF);
    postStr += "&field2=";
    postStr += String(humid);
    postStr += "&field3=";
    postStr += String(pressure);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: " + String(server) + "\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

  }
  else {
    Serial.println("Connection error");
  }
  
  // catch response
  // 200 indicates that the server has accepted the response
  client.parseFloat();
  String resp = String(client.parseInt());
  Serial.println("Sending data to ThingSpeak");
  Serial.println("Response code: " + resp);
  if (resp[0] == '2') {
    Serial.println("Sent data to ThingSpeak");
    printValues(tempF, humid, pressure);
  } else {
    Serial.println("couldn't Send Data to thingspeak: " + resp);
  }

  client.stop();
}

void printValues (float tempF, float pressure, float humid) {
  Serial.println("Here's the weather...");

  Serial.print("\tTemperature = ");
  Serial.print(tempF);
  Serial.println(" *F");

  Serial.print("\tPressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("\tHumidity = ");
  Serial.print(humid);
  Serial.println(" %");
  Serial.println();
}

float convertF (float c) {
  return (c * 9.0) / 5.0 + 32;
}
