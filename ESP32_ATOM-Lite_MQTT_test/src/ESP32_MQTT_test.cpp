/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-mqtt
 */

#include <M5Atom.h>
#include <WiFi.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "Secret.h"

#define SENSOR_PIN 33 // GPIO33 ATOM-Lite

const char WIFI_SSID[] = SECRET_SSID;     // CHANGE IN Secret.h file
const char WIFI_PASSWORD[] = SECRET_PASS;   // CHANGE IN Secret.h file

const char MQTT_BROKER_ADDRESS[] = SECRET_MQTT_BROKER_ADDRESS;   // CHANGE IN Secret.h file
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = SECRET_MQTT_CLIENT_ID;   // CHANGE IN Secret.h file
const char MQTT_USERNAME[] = SECRET_MQTT_USERNAME;     // CHANGE IN Secret.h file
const char MQTT_PASSWORD[] = SECRET_MQTT_PASSWORD;     // CHANGE IN Secret.h file

// The MQTT topics that ESP32 should publish/subscribe
char PUBLISH_TOPIC[80];
char SUBSCRIBE_TOPIC[80]; 
char PUBLISH_TOPIC_BUTTON[80];

const int PUBLISH_INTERVAL = 2000;  // 1 seconds. Can be anything.

WiFiClient network;
MQTTClient mqtt = MQTTClient(256);

unsigned long lastPublishTime = 0;
uint8_t FSM = 0;  // Store the number of key presses.
uint color4LED = 0x000000; // Buffer, holds the current color.
String color4Button ="red"; 

bool toggleButton=false;

//*******************************************************
// Predeclaration (Skal være her når PlatFormIO bruges og kode er i cpp)
void connectToMQTT();
void sendToMQTT();
void sendToMQTT_BTN();
void messageHandler(String &topic, String &payload);
//*******************************************************

void setup() {
  // make the final MQTT Topic strings
  strcpy(PUBLISH_TOPIC,MQTT_CLIENT_ID);
  strcat(PUBLISH_TOPIC,"/temp");    // CHANGE IT AS YOU DESIRE
  strcpy(SUBSCRIBE_TOPIC,MQTT_CLIENT_ID);
  strcat(SUBSCRIBE_TOPIC,"/receive");    // CHANGE IT AS YOU DESIRE
  strcpy(PUBLISH_TOPIC_BUTTON,MQTT_CLIENT_ID);
  strcat(PUBLISH_TOPIC_BUTTON,"/button");    // CHANGE IT AS YOU DESIRE

  M5.begin(false, false,true); 
  delay(50);
  Serial.begin(115200);

  M5.dis.drawpix(0, 0x00ff00);  // Light the LED with the specified RGB color
  // 00ff00(Atom-Matrix has only one light). RGB 0x00ff00 
  
  // set the ADC attenuation to 11 dB (up to ~3.3V input)
  //analogSetAttenuation(ADC_11db);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("ESP32ATOM - Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  connectToMQTT();
}

void loop() {
  mqtt.loop();

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    sendToMQTT();

    // Blink the Onboard LED
    M5.dis.drawpix(0, 0x00ff00);  // Light the LED with the specified RGB color (green)
    delay(100);
    M5.dis.drawpix(0, color4LED);  // Light the LED with the specified RGB color

    lastPublishTime = millis();
  }
  if (M5.Btn.wasPressed()) {  // Check if the key is pressed.
      sendToMQTT_BTN();
  }

  delay(50);
  M5.update();  // Read the press state of the key.
  
  // if (!mqtt.connected()) {
  //   Serial.println("ESP32 - MQTT broker Reconnect");
  //   connectToMQTT();
  // }
}

void connectToMQTT() {
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADDRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.print("ESP32 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("ESP32 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("ESP32 - Subscribed to the topic: ");
  else
    Serial.print("ESP32 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("ESP32 - MQTT broker Connected!");
}

void sendToMQTT() {
  JsonDocument message;
  message["timestamp"] = millis();
  message["data"] = analogRead(SENSOR_PIN);  // Or you can read data from other sensors
  char messageBuffer[512];
  serializeJson(message, messageBuffer);

  mqtt.publish(PUBLISH_TOPIC, messageBuffer);

  Serial.println("ESP32 - sent to MQTT:");
  Serial.print("- topic: ");
  Serial.println(PUBLISH_TOPIC);
  Serial.print("- payload: ");
  Serial.println(messageBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("ESP32 - received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.print("- payload: ");
  Serial.println(payload);

  FSM++;
  if (FSM >= 4) {
      FSM = 0;
  }

  switch (FSM) {
    case 0:
        M5.dis.drawpix(0, 0xffff00);  // YELLOW 
        color4LED=0xffff00;
        color4Button = "yellow";
        break;
    case 1:
        M5.dis.drawpix(0, 0xff0000);  // RED  
        color4LED=0xff0000;
        color4Button = "red";
        break;
    case 2:
        M5.dis.drawpix(0, 0x0000ff);  // BLUE 
        color4LED=0x0000ff;
        color4Button = "blue";
        break;
    case 3:
        M5.dis.drawpix(0, 0x00ff00);  // GREEN  
        color4LED=0x00ff00;
        color4Button = "green";
        break;
    default:
        break;
    }
}

void sendToMQTT_BTN() {
  if (toggleButton==true){
    mqtt.publish(PUBLISH_TOPIC_BUTTON, "green");
    toggleButton=false;  
  } 
   else{
    mqtt.publish(PUBLISH_TOPIC_BUTTON, color4Button);
    toggleButton=true;  
  } 
  Serial.println("ESP32 - sent to MQTT:");
  Serial.print("- topic: ");
  Serial.println(PUBLISH_TOPIC_BUTTON);
  Serial.print("- payload: ");
  Serial.println("Button_Pressed");
}