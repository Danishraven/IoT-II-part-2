
#include <vector>
#include "FS.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Structure to hold client device information
#include "sniffer.h"
#include <Arduino.h>

// Configuration
const int CHANNEL = 1; // WiFi channel to monitor (1-13)

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 WiFi Client Device Sniffer");
  Serial.println("=================================");
  Serial.printf("Monitoring channel: %d\n\n", CHANNEL);

  startSniffer(CHANNEL);
}

void loop()
{
  // Nothing to poll here; `sendClient` in the sniffer callback
  // prints discovered clients immediately. Keep a small delay
  // to yield CPU and allow background WiFi tasks to run.
}

