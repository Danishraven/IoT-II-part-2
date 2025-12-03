#include "meshNode.h"

extern Scheduler userScheduler;
extern painlessMesh mesh;

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
}

void meshNode::update() {
    mesh.update();
}

void meshNode::send(uint32_t timeValue, const String& text, int number) {
    String msg = String(timeValue) + "|" + text + "|" + String(number);
    mesh.sendBroadcast(msg);
}

void meshNode::sendWithNodeTime(const String& text, int number) {
    uint32_t t = mesh.getNodeTime();
    send(t, text, number);
}
