#include "meshNode.h"

extern Scheduler userScheduler;
extern painlessMesh mesh;

static meshNode* g_controllerInstance = nullptr;

void receivedCallback(uint32_t from, String &msg);

meshNode::meshNode(const std::string& meshPrefix,
                   const std::string& meshPassword,
                   int meshPort)
    : prefix(meshPrefix),
      password(meshPassword),
      port(meshPort)
{}

void meshNode::begin() {
    mesh.setDebugMsgTypes(ERROR | STARTUP);

    String pfx(prefix.c_str());
    String pwd(password.c_str());

    mesh.init(pfx, pwd, &userScheduler, port);

    mesh.onReceive(&receivedCallback);

    g_controllerInstance = this;    
}

void meshNode::update() {
    mesh.update();
}

void meshNode::send(uint32_t timeValue, const String& text, int number) {
    Serial.printf("Sending message: time=%u, text=%s, number=%d\n",
                  timeValue, text.c_str(), number);
    String msg = String(timeValue) + "|" + text + "|" + String(number);
    mesh.sendBroadcast(msg);
}

void meshNode::sendWithNodeTime(const String& text, int number) {
    Serial.println("Getting node time...");
    uint32_t t = mesh.getNodeTime();
    send(t, text, number);
}

void meshNode::onMessage(uint32_t from, const String& msg) {
    // Expect "time|text|int"
    int firstSep  = msg.indexOf('|');
    int secondSep = msg.indexOf('|', firstSep + 1);

    if (firstSep < 0 || secondSep < 0) {
        Serial.printf("Bad message format from %u: %s\n", from, msg.c_str());
        return;
    }

    String timeStr  = msg.substring(0, firstSep);
    String text     = msg.substring(firstSep + 1, secondSep);
    String valueStr = msg.substring(secondSep + 1);

    uint32_t t = (uint32_t) timeStr.toInt();
    int value  = valueStr.toInt();

    Serial.printf("Controller: from=%u, time=%u, text=%s, value=%d\n",
                  from, t, text.c_str(), value);

    // Here you put your “controller logic”:
    // - update a display
    // - log to serial
    // - forward to MQTT, etc.
}

void receivedCallback(uint32_t from, String &msg) {
    if (g_controllerInstance) {
        g_controllerInstance->onMessage(from, msg);
    }
}
