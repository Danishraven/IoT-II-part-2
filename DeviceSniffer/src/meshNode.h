#ifndef MESH_NODE_H
#define MESH_NODE_H

#include <string>
#include "painlessMesh.h"

class meshNode {
private:
    std::string prefix;
    std::string password;
    int port;

public:
    meshNode(const std::string& meshPrefix,
             const std::string& meshPassword,
             int meshPort);

    void begin();
    void update();
    void send(uint32_t timeValue, const String& text, int number);
    void sendWithNodeTime(const String& text, int number);
};
#endif
