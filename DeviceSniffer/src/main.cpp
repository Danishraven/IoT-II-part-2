
#include <vector>
#include "FS.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Structure to hold client device information
struct ClientDevice
{
  uint8_t mac[6];
  int8_t rssi;
};

// Array to store detected clients
std::vector<ClientDevice> clients;

// Configuration
const int CHANNEL = 1; // WiFi channel to monitor (1-13)

String macToString(uint8_t *mac)
{
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void sendClient(uint8_t *mac, int8_t rssi)
{

  ClientDevice Client;
  memcpy(Client.mac, mac, 6);
  Client.rssi = rssi;
  clients.push_back(Client);

  Serial.printf("DEVICE: %s | RSSI: %3d dBm\n",
                macToString(mac).c_str(), rssi);
}

void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;

  // Get RSSI
  int8_t rssi = ctrl.rssi;

  // Parse the management frame
  uint8_t *payload = pkt->payload;
  uint8_t frameType = payload[0];
  uint8_t frameSubType = (payload[0] & 0xF0) >> 4;

  // We're interested in probe requests (subtype 4)
  if (frameSubType == 4)
  {
    // Extract source MAC address (transmitter address)
    uint8_t *srcMac = &payload[10];

    // Skip random/locally administered MAC addresses (privacy protection)
    if (srcMac[0] & 0x02)
      return;

    // Send client
    sendClient(srcMac, rssi);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 WiFi Client Device Sniffer");
  Serial.println("=================================");
  Serial.printf("Monitoring channel: %d\n", CHANNEL);
  Serial.println();

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize promiscuous mode
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuousCallback);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
}

void loop()
{
}
