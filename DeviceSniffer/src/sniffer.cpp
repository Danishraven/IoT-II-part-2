#include "sniffer.h"
#include <vector>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

static std::vector<ClientDevice> clients;

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

  Serial.printf("DEVICE: %s | RSSI: %3d dBm\n", macToString(mac).c_str(), rssi);
}

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
    sendClient(srcMac, rssi);
  }
}

void startSniffer(int channel)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuousCallback);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}
