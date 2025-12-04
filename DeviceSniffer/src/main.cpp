#include "deviceData.h"
#include <time.h>

#include <vector>
#include "FS.h"
#include "meshNode.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Structure to hold client device information
#include <Arduino.h>
#include "mqttHandler.h"

Scheduler userScheduler;
painlessMesh mesh;

bool isController = true;

meshNode myMesh("MeshPrefix", "MeshPassword", 5555, isController, "controllerNode");

unsigned long lastCall = 0;
const unsigned long interval = 60000;

struct SeenMac
{
    String mac;
    uint32_t lastSeenMs;
};

std::vector<SeenMac> seenMacs;

const uint32_t DUP_WINDOW_MS = 500;

struct ClientDevice
{
    uint8_t mac[6];
    int8_t rssi;
};

std::vector<ClientDevice> clients;

// Configuration
const int CHANNEL = 1; // WiFi channel to monitor (1-13)
static String macToString(const uint8_t *mac)
{
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

volatile bool hasNewClient = false;
ClientDevice lastClient;

static void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;

    int8_t rssi = ctrl.rssi;

    uint8_t *payload = pkt->payload;
    uint8_t frameSubType = (payload[0] & 0xF0) >> 4;

    // Probe request subtype
    if (frameSubType == 4)
    {
        uint8_t *srcMac = &payload[10];

        // Copy into a global buffer (no vector, no String)
        memcpy(lastClient.mac, srcMac, 6);
        lastClient.rssi = rssi;
        hasNewClient = true;
    }
}

void startSniffer(int channel)
{
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuousCallback);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

String createJsonArray(const std::vector<deviceData> &devices);

void setup()
{
    Serial.begin(115200);
    if (isController)
    {
        initWiFi();
        stopWiFi();
    }
    myMesh.begin();
    startSniffer(CHANNEL);
}

void loop()
{
    myMesh.update();

    if (isController)
    {
        unsigned long timerNow = millis();

        if (timerNow - lastCall > interval)
        {
            Serial.println(myMesh.devices.size());
            if (myMesh.devices.size() != 0)
            {
                myMesh.stopMesh(); // Stop mesh to safely access devices
                startWifi();    // Restart WiFi to send MQTT
                String payload = createJsonArray(myMesh.devices);
                sendToMQTT(payload);
                myMesh.devices.clear(); // Clear after sending
                stopWiFi();     // Stop WiFi after sending
                myMesh.begin(); // Restart mesh
                startSniffer(CHANNEL);
            }
            lastCall = millis();
        }
    }

    if (!hasNewClient)
    {
        return;
    }

    hasNewClient = false;

    String macStr = macToString(lastClient.mac);
    int8_t rssi = lastClient.rssi;
    uint32_t lastSeenNow = millis();

    bool seenBefore = false;

    for (auto &entry : seenMacs)
    {
        if (entry.mac == macStr)
        {
            seenBefore = true;
            uint32_t dt = lastSeenNow - entry.lastSeenMs;

            if (dt < DUP_WINDOW_MS)
            {
                return;
            }

            entry.lastSeenMs = lastSeenNow;
            break;
        }
    }

    if (!seenBefore)
    {
        SeenMac entry;
        entry.mac = macStr;
        entry.lastSeenMs = lastSeenNow;
        seenMacs.push_back(entry);
    }

    myMesh.sendDataToMesh(macStr, rssi);
    clients.push_back(lastClient);
}

String createJsonArray (const std::vector<deviceData> &devices)
{
    String json = "[";
    for (size_t i = 0; i < devices.size(); i++)
    {
        const deviceData &dev = devices[i];
        json += "{";
        json += "\"mac\":\"" + dev.getMac() + "\",";
        json += "\"X\":" + String(dev.lastArea.center.x) + ",";
        json += "\"Y\":" + String(dev.lastArea.center.y) + ",";
        json += "\"errorRadius\":" + String(dev.lastArea.radius) + ",";
        json += "\"timestamp\":" + dev.getTimestamp();
        json += "}";
        if (i < devices.size() - 1)
        {
            json += ",";
        }
    }
    json += "]";
    return json;
}