#include <Arduino.h>
#include "deviceData.h"
#include <time.h>

void setup() {
    Serial.begin(115200);
    deviceData data = deviceData("AA:BB:CC:DD:EE:FF", time(nullptr), -50, -55, -60);
    Serial.println("Device data processed.");
    Serial.printf("MAC Address: %s\n", "AA:BB:CC:DD:EE:FF");
    Serial.printf("Timestamp: %ld\n", time(nullptr));
    Serial.printf("RSSI Values: %d, %d, %d\n", -50, -55, -60);
    deviceData::Point point = data.trilaterate(0.0f, 0.0f,
                                                 4.0f, 0.0f,
                                                 0.0f, 3.0f);
    Serial.printf("Estimated Position: (%.2f, %.2f) \n",
                  point.x, point.y);
    Serial.println("Setup complete.");
}

void loop() {
    // Your loop code here
}