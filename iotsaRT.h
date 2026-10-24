#ifndef _IOTSART_H_
#define _IOTSART_H_
#include "iotsa.h"
#include "iotsaApi.h"

typedef enum {stim_rise, stim_fall, stim_toggle} stimulusType;
typedef enum {resp_same, resp_reverse, resp_rise, resp_fall} responseType;

//
// Response time module. A stimulus is an edge on outPin; the measurement ends
// when inPin reaches the expected level (done), after the timeout (timeout),
// or with a reset (cancelled). Each stimulus gets the next count, so a client
// can tell its own measurement from an older one or someone else's.
//
// /api/rtconfig: the settings. /api/stimulus: PUT {"fire": true} fires,
// {"reset": true} cancels a waiting measurement and puts the output at its
// rest level; GET says whether a stimulus can fire now. /api/response: the
// last measurement, with the settings it was taken with. Everything is open
// to everyone on the network: it is a bench instrument.
//
class IotsaRTMod : public IotsaModule {
public:
  IotsaRTMod(IotsaApplication& _app, int _outPin, int _inPin)
  : IotsaModule(_app),
    outPin(_outPin),
    inPin(_inPin)
  {}
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
protected:
  void getHandler(IotsaApiContext& ctx) override;
  void putHandler(IotsaApiContext& ctx) override;
#ifdef IOTSA_WITH_WEB
  String webTitle() override { return "Response Time"; }
  void formHandler_args(IotsaWebContext& ctx) override;
  void formHandler_fields(IotsaWebContext& ctx) override;
#endif
  void configLoad() override;
  void configSave() override;

private:
  struct Settings {
    stimulusType stimulus = stim_rise;
    responseType response = resp_same;
    int duration = 0;       // ms until a rise or fall goes back to rest
    int timeout = 10000;    // ms until a measurement gives up, 0: never
  };
  typedef enum {st_waiting, st_done, st_timeout, st_cancelled} measurementState;

  const char *_notReady();
  bool _fire(IotsaContext& ctx);
  void _reset();
  void _rest();
  void _setOutput(bool hi);
  void _settingsChanged(stimulusType oldStimulus);
  String _resultText();

  int outPin;
  int inPin;
  Settings settings;
  Settings measured;        // the settings of the last stimulus
  uint32_t count = 0;       // stimuli since boot, 0: none yet
  measurementState state = st_done;  // with count 0: no stimulus yet
  bool outputHi = false;
  bool resetPending = false;  // a rise or fall goes back to rest after duration
  bool responseWanted = false;
  unsigned long stimMillis = 0;
  unsigned long stimMicros = 0;
  unsigned long respMillis = 0;
  unsigned long respMicros = 0;
};

#endif
