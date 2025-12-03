#ifndef SNIFFER_H
#define SNIFFER_H

#include <stdint.h>
#include <vector>

struct ClientDevice {
  uint8_t mac[6];
  int8_t rssi;
  // Epoch timestamp in microseconds (UTC) when available. If the device
  // RTC/NTP is not set, this will contain the ESP32 monotonic
  // microsecond clock (since boot).
  uint64_t epoch_us;
};

// Start promiscuous sniffer on given WiFi channel (1-13)
void startSniffer(int channel);

#endif // SNIFFER_H
