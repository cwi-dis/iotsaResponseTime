#include "iotsaRT.h"
#include "iotsaConfigFile.h"

//
// Names of the settings and states, as in the API and the config file.
//
static const char * const stimNames[] = {"rise", "fall", "toggle"};
static const int nStim = 3;
static const char * const respNames[] = {"same", "reverse", "rise", "fall"};
static const int nResp = 4;
static const char * const stateNames[] = {"waiting", "done", "timeout", "cancelled"};

static int lookup(const char * const *names, int count, const String& s) {
  for (int i = 0; i < count; i++) {
    if (s == names[i]) return i;
  }
  return -1;
}

// micros() wraps after 71 minutes: a longer response time only has millis.
static bool microsValid(unsigned long ms) {
  return ms < 4294000UL;
}

//
// Stimulus and response. The input is polled from the loop, so the
// resolution is a loop iteration: milliseconds, see readme.md.
//

// Why a stimulus can't fire now, or nullptr.
const char *IotsaRTMod::_notReady() {
  if (state == st_waiting) return "The previous stimulus is still waiting for its response.";
  if (resetPending) return "The output is not back at its rest level yet.";
  if (settings.stimulus == stim_rise && outputHi) return "The output is high: reset it first.";
  if (settings.stimulus == stim_fall && !outputHi) return "The output is low: reset it first.";
  return nullptr;
}

bool IotsaRTMod::_fire(IotsaContext& ctx) {
  const char *why = _notReady();
  if (why) {
    ctx.error(409, why);
    return false;
  }
  measured = settings;
  bool hi;
  if (settings.stimulus == stim_rise) {
    hi = true;
  } else if (settings.stimulus == stim_fall) {
    hi = false;
  } else {
    hi = !outputHi;
  }
  if (settings.response == resp_same) {
    responseWanted = hi;
  } else if (settings.response == resp_reverse) {
    responseWanted = !hi;
  } else {
    responseWanted = settings.response == resp_rise;
  }
  count++;
  state = st_waiting;
  stimMillis = millis();
  stimMicros = micros();
  _setOutput(hi);
  resetPending = settings.stimulus != stim_toggle;
  return true;
}

// Cancel a waiting measurement, and put the output at its rest level.
void IotsaRTMod::_reset() {
  if (state == st_waiting) state = st_cancelled;
  resetPending = false;
  _rest();
}

// Low for rise, high for fall; toggle has no rest level.
void IotsaRTMod::_rest() {
  if (settings.stimulus == stim_rise) _setOutput(false);
  if (settings.stimulus == stim_fall) _setOutput(true);
}

void IotsaRTMod::_setOutput(bool hi) {
  outputHi = hi;
  digitalWrite(outPin, hi ? HIGH : LOW);
}

// A new stimulus type has a new rest level. Not while waiting: that would
// be a stimulus of its own.
void IotsaRTMod::_settingsChanged(stimulusType oldStimulus) {
  if (settings.stimulus == oldStimulus || (state == st_waiting)) return;
  resetPending = false;
  _rest();
}

void IotsaRTMod::loop() {
  if (state == st_waiting) {
    bool input = digitalRead(inPin) == HIGH;
    if (input == responseWanted) {
      respMicros = micros();
      respMillis = millis();
      state = st_done;
    } else if (measured.timeout > 0 && millis() - stimMillis >= (unsigned long)measured.timeout) {
      state = st_timeout;
    }
  }
  // millis() - stimMillis is right even when millis() wraps around.
  if (resetPending && millis() - stimMillis >= (unsigned long)measured.duration) {
    resetPending = false;
    _setOutput(!outputHi);
  }
}

String IotsaRTMod::_resultText() {
  if (count == 0) return "none yet";
  String s = String("#") + count + ": ";
  switch (state) {
  case st_waiting:
    return s + "waiting";
  case st_timeout:
    return s + "no response within " + measured.timeout + " ms";
  case st_cancelled:
    return s + "cancelled";
  default:
    break;
  }
  unsigned long ms = respMillis - stimMillis;
  if (microsValid(ms)) return s + String((respMicros - stimMicros) / 1000.0, 3) + " ms";
  return s + ms + " ms";
}

//
// API
//
void IotsaRTMod::getHandler(IotsaApiContext& ctx) {
  if (ctx.path() == "/api/stimulus") {
    ctx.show("ready", _notReady() == nullptr);
    ctx.show("output", outputHi ? "high" : "low");
    return;
  }
  if (ctx.path() == "/api/response") {
    ctx.show("count", count);
    if (count == 0) return;
    ctx.show("state", stateNames[state]);
    if (state == st_done) {
      unsigned long ms = respMillis - stimMillis;
      ctx.show("millis", ms);
      if (microsValid(ms)) ctx.show("micros", respMicros - stimMicros);
    }
    ctx.show("stimulus", stimNames[measured.stimulus]);
    ctx.show("response", respNames[measured.response]);
    ctx.show("duration", measured.duration);
    ctx.show("timeout", measured.timeout);
    return;
  }
  ctx.show("stimulus", stimNames[settings.stimulus]);
  ctx.show("response", respNames[settings.response]);
  ctx.show("duration", settings.duration);
  ctx.show("timeout", settings.timeout);
}

void IotsaRTMod::putHandler(IotsaApiContext& ctx) {
  if (ctx.path() == "/api/stimulus") {
    bool reset = ctx.argAction("reset", IotsaContext::Transient);
    bool fire = ctx.argAction("fire", IotsaContext::Transient);
    if (ctx.failed()) return;
    if (!reset && !fire) {
      ctx.error(400, "nothing to do: fire or reset");
      return;
    }
    if (reset) _reset();
    if (fire && !_fire(ctx)) return;
    ctx.show("count", count);
    return;
  }
  stimulusType oldStimulus = settings.stimulus;
  String s = stimNames[settings.stimulus];
  if (ctx.arg("stimulus", s, IotsaContext::Saved)) {
    int i = lookup(stimNames, nStim, s);
    if (i < 0) {
      ctx.error(400, "stimulus: rise, fall or toggle");
    } else {
      settings.stimulus = (stimulusType)i;
    }
  }
  s = respNames[settings.response];
  if (ctx.arg("response", s, IotsaContext::Saved)) {
    int i = lookup(respNames, nResp, s);
    if (i < 0) {
      ctx.error(400, "response: same, reverse, rise or fall");
    } else {
      settings.response = (responseType)i;
    }
  }
  int v = settings.duration;
  if (ctx.arg("duration", v, IotsaContext::Saved)) {
    if (v < 0) {
      ctx.error(400, "duration: not negative");
    } else {
      settings.duration = v;
    }
  }
  v = settings.timeout;
  if (ctx.arg("timeout", v, IotsaContext::Saved)) {
    if (v < 0) {
      ctx.error(400, "timeout: not negative");
    } else {
      settings.timeout = v;
    }
  }
  _settingsChanged(oldStimulus);
}

//
// Web page: the measurement and its buttons, then the settings.
//
#ifdef IOTSA_WITH_WEB
void IotsaRTMod::formHandler_args(IotsaWebContext& ctx) {
  stimulusType oldStimulus = settings.stimulus;
  int i = settings.stimulus;
  if (ctx.argChoice("stimulus", i, nStim, IotsaWebContext::Saved)) settings.stimulus = (stimulusType)i;
  i = settings.response;
  if (ctx.argChoice("response", i, nResp, IotsaWebContext::Saved)) settings.response = (responseType)i;
  int v = settings.duration;
  if (ctx.arg("duration", v, IotsaWebContext::Saved)) {
    if (v < 0) {
      ctx.error("The reset duration can't be negative.");
    } else {
      settings.duration = v;
    }
  }
  v = settings.timeout;
  if (ctx.arg("timeout", v, IotsaWebContext::Saved)) {
    if (v < 0) {
      ctx.error("The timeout can't be negative.");
    } else {
      settings.timeout = v;
    }
  }
  _settingsChanged(oldStimulus);
  bool reset = ctx.argAction("reset", IotsaWebContext::Transient);
  bool fire = ctx.argAction("fire", IotsaWebContext::Transient);
  if (ctx.failed()) return;
  if (reset) _reset();
  if (fire && _fire(ctx)) ctx.feedback(String("Stimulus #") + count + " fired.");
}

void IotsaRTMod::formHandler_fields(IotsaWebContext& ctx) {
  if (ctx.includesEveryday()) {
    ctx.showStatus("Last measurement", "state", _resultText());
    const char *why = _notReady();
    ctx.showStatus("Stimulus", "ready", why ? why : "ready");
    ctx.showButton("fire", "Fire", IotsaWebContext::Transient);
    ctx.showButton("reset", "Reset", IotsaWebContext::Transient);
  }
  if (ctx.includesSettings()) {
    IotsaWebContext::Form form(ctx, "Save", "Settings");
    ctx.showChoice("Stimulus", "stimulus", settings.stimulus, stimNames, nStim, IotsaWebContext::Saved);
    ctx.showChoice("Response", "response", settings.response, respNames, nResp, IotsaWebContext::Saved);
    ctx.showField("Reset duration", "duration", settings.duration, IotsaWebContext::Saved, "ms, then a rise or fall goes back to rest");
    ctx.showField("Timeout", "timeout", settings.timeout, IotsaWebContext::Saved, "ms, 0: wait forever");
  }
}
#endif // IOTSA_WITH_WEB

void IotsaRTMod::setup() {
  pinMode(outPin, OUTPUT);
  pinMode(inPin, INPUT_PULLUP);
  configLoad();
  _rest();
}

void IotsaRTMod::lateSetup() {
  // "rtconfig" is the settings and the web page; "stimulus" and "response"
  // are the measurement, with no page of their own.
  api.setup("rtconfig", true, true);
  api.setup("stimulus", true, true, false, false);
  api.setup("response", true, false, false, false);
  name = "rtconfig";
}

void IotsaRTMod::configLoad() {
  IotsaConfigFileLoad cf("/config/RT.cfg");
  String s;
  cf.get("stimulus", s, "rise");
  int i = lookup(stimNames, nStim, s);
  settings.stimulus = i < 0 ? stim_rise : (stimulusType)i;
  cf.get("response", s, "same");
  i = lookup(respNames, nResp, s);
  settings.response = i < 0 ? resp_same : (responseType)i;
  cf.get("duration", settings.duration, 0);
  cf.get("timeout", settings.timeout, 10000);
}

void IotsaRTMod::configSave() {
  IotsaConfigFileSave cf("/config/RT.cfg");
  cf.put("stimulus", stimNames[settings.stimulus]);
  cf.put("response", respNames[settings.response]);
  cf.put("duration", settings.duration);
  cf.put("timeout", settings.timeout);
}

String IotsaRTMod::info() {
  return "<p>See <a href=\"/rtconfig\">/rtconfig</a> to fire a stimulus, see the response time and change the settings. "
    "REST: <a href=\"/api/rtconfig\">/api/rtconfig</a> for the settings, PUT <a href=\"/api/stimulus\">/api/stimulus</a> "
    "to fire a stimulus or reset, <a href=\"/api/response\">/api/response</a> for the response time.</p>";
}
