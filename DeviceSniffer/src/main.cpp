#include <Arduino.h>
#include "FS.h"
#include "meshNode.h"

Scheduler  userScheduler;
painlessMesh Mesh;

meshNode myMesh("MeshPrefix", "MeshPassword", 5555);

void setup() {
  Serial.begin(115200);
  myMesh.begin();
}

void loop() {
  myMesh.update();
}
