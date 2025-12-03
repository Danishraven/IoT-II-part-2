
#include <vector>
#include "FS.h"
#include "meshNode.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Structure to hold client device information
#include "sniffer.h"
#include <Arduino.h>

Scheduler userScheduler;
painlessMesh Mesh;

meshNode myMesh("MeshPrefix", "MeshPassword", 5555);

// Configuration
const int CHANNEL = 1; // WiFi channel to monitor (1-13)

void setup()
{
  Serial.begin(115200);
  myMesh.begin();
}

void loop()
{
  myMesh.update();
}
