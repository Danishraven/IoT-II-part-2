#include "meshNode.h"
#include "deviceData.h"
#include <vector>

extern Scheduler userScheduler;
extern painlessMesh mesh;

struct coordinates
{
    float x;
    float y;
};

static meshNode *g_controllerInstance = nullptr;
static meshNode *g_clientInstance = nullptr;

const coordinates snifferPositions[3] = {
    {0.0f, 0.0f}, // controllerNode
    {5.0f, 0.0f}, // Node1
    {2.5f, 4.33f} // Node2
};


void receivedCallback(uint32_t from, String &msg);

meshNode::meshNode(const std::string &meshPrefix,
                   const std::string &meshPassword,
                   int meshPort,
                   bool meshIsController,
                   const std::string &meshDeviceName)
    : prefix(meshPrefix),
      password(meshPassword),
      port(meshPort),
      isController(meshIsController),
      deviceName(meshDeviceName)
{
}

void meshNode::begin()
{
    mesh.setDebugMsgTypes(ERROR | STARTUP);

    String pfx(prefix.c_str());
    String pwd(password.c_str());

    mesh.init(pfx, pwd, &userScheduler, port);

    mesh.onReceive(&receivedCallback);

    if (isController)
    {
        g_controllerInstance = this;
    }
    else
    {
        g_clientInstance = this;
    }
}

void meshNode::stopMesh()
{
    // Only meaningful on this ESP
    mesh.stop();              // painlessMesh: stop all mesh activity
    g_controllerInstance = nullptr;  // optional: clear pointer if this was controller
    g_clientInstance     = nullptr;  // optional if you want to be strict
}


void meshNode::update()
{
    mesh.update();
}

void meshNode::formatData(String &out, uint32_t timeValue, const String &text, int number)
{
    out = String(deviceName.c_str()) + "|" + String(timeValue) + "|" + text + "|" + String(number);
}

void meshNode::send(uint32_t timeValue, const String &text, int number)
{
    String msg;
    formatData(msg, timeValue, text, number);
    mesh.sendBroadcast(msg);
    Serial.printf("Sent: %s\n", msg.c_str());
}

void meshNode::sendWithNodeTime(const String &text, int number)
{
    uint32_t t = mesh.getNodeTime();
    send(t, text, number);
}

void meshNode::controllerHandleMessage(const String &text, int number)
{
    String msg;
    formatData(msg, mesh.getNodeTime(), text, number);
    onMessage(msg);
}

void meshNode::sendDataToMesh(const String &text, int number)
{
    if (g_controllerInstance)
    {
        g_controllerInstance->controllerHandleMessage(text, number);
    }
    else if (g_clientInstance)
    {
        g_clientInstance->sendWithNodeTime(text, number);
    }
}

void meshNode::onMessage(const String &msg)
{
    // Expect "deviceName|time|text|int"
    int firstSep = msg.indexOf('|');                // after deviceName
    int secondSep = msg.indexOf('|', firstSep + 1); // after time
    int thirdSep = msg.indexOf('|', secondSep + 1); // after text

    if (firstSep < 0 || secondSep < 0 || thirdSep < 0)
    {
        Serial.printf("Bad message format: %s\n", msg.c_str());
        return;
    }

    String deviceStr = msg.substring(0, firstSep);
    String timeStr = msg.substring(firstSep + 1, secondSep);
    String text = msg.substring(secondSep + 1, thirdSep); // e.g. MAC
    String valueStr = msg.substring(thirdSep + 1);        // e.g. RSSI

    uint32_t t = (uint32_t)timeStr.toInt();
    int value = valueStr.toInt();

    Serial.printf("Controller: dev=%s, time=%u, text=%s, value=%d\n",
                  deviceStr.c_str(), t, text.c_str(), value);

    if (isController)
    {
        // text = MAC string, value = RSSI
        addSnifferSample(text, (int8_t)value, deviceStr);
    }
}

void receivedCallback(uint32_t from, String &msg)
{
    if (g_controllerInstance)
    {
        g_controllerInstance->onMessage(msg);
    }
}

int meshNode::deviceSlotFromName(const String &devName)
{
    if (devName == "controllerNode")
        return 0;
    if (devName == "Node1")
        return 1;
    if (devName == "Node2")
        return 2;

    // Unknown device – ignore
    return -1;
}

void meshNode::addSnifferSample(const String &mac, int8_t rssi, const String &devName)
{
    int slot = deviceSlotFromName(devName);
    if (slot < 0 || slot > 2)
    {
        // Unknown device name, ignore
        return;
    }

    // Find existing MAC
    for (size_t i = 0; i < macSlotCount; i++)
    {
        RssiSlots &ms = macSlots[i];
        if (ms.mac == mac)
        {
            ms.rssi[slot] = rssi;
            ms.filled[slot] = true;

            // If all three devices have reported, call your function
            if (ms.filled[0] && ms.filled[1] && ms.filled[2])
            {
                deviceData data = deviceData(ms.mac,
                                             ms.rssi[0],
                                             ms.rssi[1],
                                             ms.rssi[2]);
                auto p = data.trilaterate(snifferPositions[0].x, snifferPositions[0].y,
                                          snifferPositions[1].x, snifferPositions[1].y,
                                          snifferPositions[2].x, snifferPositions[2].y);
                data.lastPoint = p;
                this->devices.push_back(data);
                // Reset for next round for this MAC
                ms.filled[0] = ms.filled[1] = ms.filled[2] = false;
            }
            return;
        }
    }

    // Not found → create new entry
    if (macSlotCount < MAX_MACS)
    {
        RssiSlots &ms = macSlots[macSlotCount++];
        ms.mac = mac;
        // initialize filled/rssi with defaults already set in struct
        ms.rssi[slot] = rssi;
        ms.filled[slot] = true;
    }
}
