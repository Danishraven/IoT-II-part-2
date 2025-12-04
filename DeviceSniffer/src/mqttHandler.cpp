#include "mqttHandler.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>

#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Configuration values (single definition)
const char *WIFI_SSID = "TEC-IOT";
const char *WIFI_PASSWORD = "42090793";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const char *MQTT_SERVER = "wilson.local";
const int MQTT_PORT = 8883;
const char *MQTT_USER = "elev1";
const char *MQTT_PASS = "password";
const char *MQTT_TOPIC = "esp32/alex_alexander_nora/sniffer";

String getLocalTimeString()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return String(); // empty -> indicates failure
    }
    char buf[32];
    if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo) == 0)
    {
        return String();
    }
    return String(buf);
}

String getLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return String("");
    }
    String timeString = asctime(&timeinfo);
    return timeString;
}

void startWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void initWiFi()
{
    startWifi();

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    getLocalTime();
}

void stopWiFi()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void sendToMQTT(String payload)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        initWiFi();
    }
    WiFiClientSecure espClient;
    PubSubClient mqttClient(espClient);
    // Configure MQTT over TLS
    Serial.println("setting client to insecure");
    espClient.setInsecure();
    // espClient.setCACert(MQTT_CA_CERT);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT over TLS...");

        String clientId = "ESP32-" + String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" - retrying in 5 seconds");
            delay(5000);
        }
    }

    mqttClient.setBufferSize(65535);
    mqttClient.publish(MQTT_TOPIC, payload.c_str());
}
