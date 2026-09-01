//
// iotsaResponseTime: a WiFi HTTP server that fires a configurable digital edge
// on one GPIO and measures how long until an expected edge appears on another.
// Built for bench work where millisecond resolution is enough (originally: timing
// experiments on pneumatic logic). See readme.md.
//

#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"
#include "iotsaLed.h"
#include "iotsaRT.h"

#define NEOPIXEL_PIN 15
#define OUTPUT_PIN 4
#define INPUT_PIN 5

IotsaApplication application("Iotsa Response time Server");
IotsaWifiMod wifiMod(application);
IotsaRTMod rtMod(application, OUTPUT_PIN, INPUT_PIN);
IotsaOtaMod otaMod(application);
IotsaLedMod ledMod(application, NEOPIXEL_PIN);

void setup(void){
  application.setup();
  application.lateSetup();
}

void loop(void){
  application.loop();
}
