//
// iotsaResponseTime: a WiFi HTTP server that fires a configurable digital edge
// on one GPIO and measures how long until an expected edge appears on another.
// Built for bench work where millisecond resolution is enough (originally: timing
// experiments on pneumatic logic). See readme.md.
//

#include "iotsa.h"
#include "iotsaRT.h"

#define OUTPUT_PIN 4
#define INPUT_PIN 5

IotsaApplication application("Iotsa Response time Server");
IotsaRTMod rtMod(application, OUTPUT_PIN, INPUT_PIN);

void setup(void){
  application.setup();
  application.lateSetup();
}

void loop(void){
  application.loop();
}
