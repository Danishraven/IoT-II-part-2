#ifndef MESH_NODE_H
#define MESH_NODE_H

#include <string>
#include "painlessMesh.h"

class meshNode
{
private:
    std::string prefix;
    std::string password;
    int port;
    bool isController;
    std::string deviceName;

    struct RssiSlots
    {
        String mac;
        int8_t rssi[3] = {0, 0, 0};
        bool filled[3] = {false, false, false};
    };

    static const size_t MAX_MACS = 64;
    RssiSlots macSlots[MAX_MACS];
    size_t macSlotCount = 0;

    int deviceSlotFromName(const String &devName);
    void addSnifferSample(const String &mac, int8_t rssi, const String &devName);

public:
    std::vector<deviceData> devices;
    meshNode(const std::string &meshPrefix,
             const std::string &meshPassword,
             int meshPort,
             bool isController,
             const std::string &deviceName);

    void begin();
    void update();
    void formatData(String &out, uint32_t timeValue, const String &text, int number);
    void send(uint32_t timeValue, const String &text, int number);
    void sendWithNodeTime(const String &text, int number);
    void sendDataToMesh(const String &text, int number);
    void controllerHandleMessage(const String &text, int number);

    void onMessage(const String &msg);
};
#endif
