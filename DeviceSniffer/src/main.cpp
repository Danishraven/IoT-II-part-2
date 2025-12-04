
#include <vector>
#include "FS.h"
#include "meshNode.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Structure to hold client device information
#include "sniffer.h"
#include <Arduino.h>
#include "mqttHandler.h"

Scheduler userScheduler;
painlessMesh mesh;

meshNode myMesh("MeshPrefix", "MeshPassword", 5555);

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

static void sendClient(const uint8_t *mac, int8_t rssi)
{
  ClientDevice c;
  memcpy(c.mac, mac, 6);
  c.rssi = rssi;
  clients.push_back(c);

  myMesh.sendWithNodeTime(macToString(mac), rssi);
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

void setup()
{
    Serial.begin(115200);
    int mil = millis();
    sendToMQTT("TEST1");
    Serial.printf("MQTT send took %d ms\n", millis() - mil);
    mil = millis();
    sendToMQTT("TEST2");
    Serial.printf("MQTT send took %d ms\n", millis() - mil);
    mil = millis();
    sendToMQTT("TEST3");
    Serial.printf("MQTT send took %d ms\n", millis() - mil);
//   myMesh.begin();
//   startSniffer(CHANNEL);
  delay(2000000);
}

void loop()
{
  myMesh.update();

  static unsigned long lastTick = 0;
  if (millis() - lastTick > 1000) {
    lastTick = millis();
  }

  if (hasNewClient) {
    hasNewClient = false;

    // Now we are in normal Arduino context: safe to use String, vector, mesh send, etc.
    String macStr = macToString(lastClient.mac);
    myMesh.sendWithNodeTime(macStr, lastClient.rssi);

    // Optional: also push into your vector here
    clients.push_back(lastClient);
  }
}
