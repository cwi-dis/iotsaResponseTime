#ifndef _IOTSART_H_
#define _IOTSART_H_
#include "iotsa.h"
#include "iotsaApi.h"

typedef enum {stim_rise, stim_fall, stim_toggle} stimulusType;
typedef enum {resp_same, resp_reverse, resp_rise, resp_fall} responseType;

class IotsaRTMod : public IotsaApiMod {
public:
  IotsaRTMod(IotsaApplication& _app, int _outPin, int _inPin)
  : IotsaApiMod(_app),
    outPin(_outPin),
    inPin(_inPin),
    stimulus(stim_rise),
    response(resp_same),
    duration(0),
    trigger(false)
  {}
  void setup() override;
  void serverSetup() override;
  void loop() override;
  String info() override;
protected:
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  void configLoad() override;
  void configSave() override;
  void handler();
  bool canDoStimulus();
  void doStimulus();

  int outPin;
  int inPin;
  stimulusType stimulus;
  responseType response;
  int duration;
  bool trigger;
};

#endif
