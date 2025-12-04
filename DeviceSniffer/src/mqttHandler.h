// mqttHandler.h
// Declarations for MQTT and time helper routines.

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

// Configuration variables (defined in mqttHandler.cpp)
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;

extern const char *ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

extern const char *MQTT_SERVER;
extern const int MQTT_PORT;
extern const char *MQTT_USER;
extern const char *MQTT_PASS;
extern const char *MQTT_TOPIC;

// Utility functions (definitions in mqttHandler.cpp)
String getLocalTimeString();
String getLocalTime();
void initWiFi();
void sendToMQTT(String payload);

#endif // MQTT_HANDLER_H