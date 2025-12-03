#include <Arduino.h>
#include "FS.h"
#include "meshNode.h"

Scheduler  userScheduler;
painlessMesh Mesh;

meshNode myMesh("MeshPrefix", "MeshPassword", 5555);

// put function declarations here:
String readFile(fs::FS &fs, const char * path);
bool writeFile(fs::FS &fs, const char * path, const String message);
bool appendFile(fs::FS &fs, const char * path, const String message);
bool deleteFile(fs::FS &fs, const char * path);

// void setup() {
//   // put your setup code here, to run once:
// }

// void loop() {
//   // put your main code here, to run repeatedly:
// }

String readFile(fs::FS &fs, const char * path){
  File file = fs.open(path);
  if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return String();
  }
  Serial.println("Reading file");
  String content;
    while(file.available()){
      int c = file.read();
      if (c < 0) break;
      content += static_cast<char>(c);
    }
  file.close();
  return content;
}

bool writeFile(fs::FS &fs, const char * path, const String message){
  File file = fs.open(path, FILE_WRITE);
  if(!file){
      Serial.println("- failed to open file for writing");
      return false;
  }
  if(!file.print(message)){
      Serial.println("- write failed");
  }
  file.close();
  return true;
}

bool appendFile(fs::FS &fs, const char * path, const String message){
  if (message == "")
  {
    return true;
  }
  
  File file = fs.open(path, FILE_APPEND);
  if(!file){
      Serial.println("- failed to open file for appending");
      while (!writeFile(fs, path, message))
      
      return false;
  }
  if(!file.print(message)){
      Serial.println("- append failed");
  }
  file.close();
  return true;
}

bool deleteFile(fs::FS &fs, const char * path){
  if(!fs.remove(path)){
      Serial.println("- delete failed");
      return false;
  }
  return true;
}

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// Structure to hold client device information
struct ClientDevice {
  uint8_t mac[6];
  int8_t rssi;
  uint32_t lastSeen;
  uint32_t packetCount;
  String ssid;
};

// Array to store detected clients
const int MAX_CLIENTS = 50;
ClientDevice clients[MAX_CLIENTS];
int clientCount = 0;

// Configuration
const int CHANNEL = 1;  // WiFi channel to monitor (1-13)
const int RSSI_THRESHOLD = -80;  // Minimum RSSI to consider
const uint32_t CLIENT_TIMEOUT = 30000;  // Remove clients not seen for 30 seconds

String macToString(uint8_t* mac) {
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}


String extractSSIDFromProbeRequest(uint8_t* payload, int len) {
  // Probe request frame structure:
  // 24 bytes: MAC header
  // Variable: Fixed parameters (12 bytes)
  // Variable: Information Elements
  
  if (len < 36) return "";
  
  uint8_t* ie = &payload[36];  // Start of Information Elements
  int remaining = len - 36;
  
  while (remaining >= 2) {
    uint8_t ieType = ie[0];
    uint8_t ieLen = ie[1];
    
    if (remaining < ieLen + 2) break;
    
    // SSID Information Element (type 0)
    if (ieType == 0 && ieLen > 0 && ieLen <= 32) {
      String ssid = "";
      for (int i = 0; i < ieLen; i++) {
        if (ie[2 + i] >= 32 && ie[2 + i] <= 126) {
          ssid += (char)ie[2 + i];
        }
      }
      return ssid;
    }
    
    ie += ieLen + 2;
    remaining -= ieLen + 2;
  }
  
  return "*"; // Broadcast probe (looking for any AP)
}

void updateClient(uint8_t* mac, int8_t rssi, String ssid) {
  // Look for existing client
  for (int i = 0; i < clientCount; i++) {
    if (memcmp(clients[i].mac, mac, 6) == 0) {
      // Update existing client
      clients[i].rssi = rssi;
      clients[i].lastSeen = millis();
      clients[i].packetCount++;
      if (ssid.length() > 0 && ssid != "*") {
        clients[i].ssid = ssid;
      }
      
      // Print real-time detection
      Serial.printf("📱 %s | RSSI: %3d dBm | Packets: %3d | SSID: %s\n",
                    macToString(mac).c_str(), rssi, clients[i].packetCount, 
                    ssid.length() > 0 ? ssid.c_str() : clients[i].ssid.c_str());
      return;
    }
  }
  
  // Add new client if we have space
  if (clientCount < MAX_CLIENTS) {
    memcpy(clients[clientCount].mac, mac, 6);
    clients[clientCount].rssi = rssi;
    clients[clientCount].lastSeen = millis();
    clients[clientCount].packetCount = 1;
    clients[clientCount].ssid = ssid.length() > 0 ? ssid : "*";
    
    Serial.printf("🆕 NEW DEVICE: %s | RSSI: %3d dBm | SSID: %s\n",
                  macToString(mac).c_str(), rssi, ssid.c_str());
    
    clientCount++;
  }
}

void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
  
  // Get RSSI
  int8_t rssi = ctrl.rssi;
  
  // Filter by RSSI threshold
  if (rssi < RSSI_THRESHOLD) return;
  
  // Parse the management frame
  uint8_t* payload = pkt->payload;
  uint8_t frameType = payload[0];
  uint8_t frameSubType = (payload[0] & 0xF0) >> 4;
  
  // We're interested in probe requests (subtype 4)
  if (frameSubType == 4) {
    // Extract source MAC address (transmitter address)
    uint8_t* srcMac = &payload[10];
    
    // Skip random/locally administered MAC addresses (privacy protection)
    if (srcMac[0] & 0x02) return;
    
    // Extract SSID from probe request
    String ssid = extractSSIDFromProbeRequest(payload, ctrl.sig_len);
    
    // Update or add client
    updateClient(srcMac, rssi, ssid);
  }
}

void cleanupOldClients() {
  uint32_t now = millis();
  int removed = 0;
  
  for (int i = 0; i < clientCount; i++) {
    if (now - clients[i].lastSeen > CLIENT_TIMEOUT) {
      // Remove this client by shifting array
      for (int j = i; j < clientCount - 1; j++) {
        clients[j] = clients[j + 1];
      }
      clientCount--;
      i--; // Check this index again
      removed++;
    }
  }
  
  if (removed > 0) {
    Serial.printf("🧹 Cleaned up %d inactive clients\n", removed);
  }
}

void printActiveClients() {
  Serial.println("\n📊 ACTIVE CLIENT DEVICES SUMMARY");
  Serial.println("=================================");
  
  if (clientCount == 0) {
    Serial.println("No active clients detected");
    Serial.println();
    return;
  }
  
  Serial.println("MAC Address       │ RSSI │ Packets │ Last Seen │ Searching For");
  Serial.println("──────────────────┼──────┼─────────┼───────────┼──────────────────");
  
  for (int i = 0; i < clientCount; i++) {
    uint32_t secondsAgo = (millis() - clients[i].lastSeen) / 1000;
    
    Serial.printf("%-17s │ %4d │ %7d │ %6ds ago │ %s\n",
                  macToString(clients[i].mac).c_str(),
                  clients[i].rssi,
                  clients[i].packetCount,
                  secondsAgo,
                  clients[i].ssid.c_str());
  }
  
  Serial.printf("\nTotal active clients: %d\n", clientCount);
  Serial.println("═══════════════════════════════════════════════════════════════\n");
}

void printHeader() {
  Serial.println("Real-time client device detection:");
  Serial.println("Format: [Device MAC] | RSSI: [signal strength] | Packets: [count] | SSID: [network name]");
  Serial.println("═══════════════════════════════════════════════════════════════");
}

void setup() {
  Serial.begin(115200);
  myMesh.begin();
  /*
  delay(1000);
  
  Serial.println("ESP32 WiFi Client Device Sniffer");
  Serial.println("=================================");
  Serial.printf("Monitoring channel: %d\n", CHANNEL);
  Serial.printf("RSSI threshold: %d dBm\n", RSSI_THRESHOLD);
  Serial.println();
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Initialize promiscuous mode
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuousCallback);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  
  Serial.println("✅ Promiscuous mode enabled - Starting to sniff client devices...\n");
  
  // Print header
  printHeader();
  */
}

void loop() {
  myMesh.update();
  /*
  // Clean up old entries every 5 seconds
  static uint32_t lastCleanup = 0;
  if (millis() - lastCleanup > 5000) {
    cleanupOldClients();
    lastCleanup = millis();
  }
 
  // Print active clients every 10 seconds
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    printActiveClients();
    lastPrint = millis();
  }
  
  delay(100);
  */
}
