/*
 * uMQTTBroker demo for Arduino
 * 
 * Minimal Demo: the program simply starts a broker and waits for any client to connect.
 */

#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"

uMQTTBroker myBroker;

/*
 * Your WiFi config here
 */
char ssid[] = "ESP8266_CU";      // your network SSID (name)
char pass[] = "RJQ-729!!"; // your network password

/*
 * WiFi init stuff
 */


void startWiFiAP()
{
  WiFi.softAP(ssid, pass);
  Serial.println("AP started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  //start the ESP as AP
  startWiFiAP();

  // Start the broker
  Serial.println("Starting MQTT broker");
  myBroker.init();
}

void loop()
{   
  // do anything here
  delay(1000);
}

