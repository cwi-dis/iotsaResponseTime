//
// Boilerplate for configurable web server (probably RESTful) running on ESP8266.
//
// The server always includes the Wifi configuration module. You can enable
// other modules with the preprocessor defines. With the default defines the server
// will allow serving of web pages and other documents, and of uploading those.
//

#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"
#include "iotsaLed.h"
#include "iotsaRT.h"


#define NEOPIXEL_PIN 15
#define OUTPUT_PIN 4
#define INPUT_PIN 5

IotsaWebServer server(80);
IotsaApplication application(server, "Iotsa Response time Server");
IotsaWifiMod wifiMod(application);
IotsaOtaMod otaMod(application);
IotsaLedMod ledMod(application, NEOPIXEL_PIN);

IotsaRTMod rtMod(application, OUTPUT_PIN, INPUT_PIN);

void setup(void){
  application.setup();
  application.serverSetup();
}
 
void loop(void){
  application.loop();
}

