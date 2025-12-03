void setup() {
    Serial.begin(115200);
    deviceData data = deviceData();
    data.setSnifferRssi("AA:BB:CC:DD:EE:FF", time(nullptr), -50, -55, -60);
    data.trilaterateArea(0.0f, 0.0f,
                         5.0f, 0.0f,
                         2.5f, 4.33f);
    Serial.println("Device data processed.");
    Serial.printf("MAC Address: %s\n", "AA:BB:CC:DD:EE:FF");
    Serial.printf("Timestamp: %ld\n", time(nullptr));
    Serial.printf("Trilateration complete.\n");
}

void loop() {
    // Your loop code here
}