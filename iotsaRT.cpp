#include "iotsaRT.h"
#include "iotsaConfigFile.h"

//
// Stimulus-response code.
// Static variables, so we can convert to interrupt-driven later.
//
static bool currentOutputHi;
static unsigned long stimMillis;
static unsigned long stimMicros;
static unsigned long stimReset;

static volatile bool responseWanted;
static volatile bool responseSeen;
static volatile unsigned long respMillis;
static volatile unsigned long respMicros;

bool IotsaRTMod::canDoStimulus() {
  // first check that we are not too early (before reset)
  if (stimulus == stim_rise && currentOutputHi) return false;
  if (stimulus == stim_fall && !currentOutputHi) return false;
  return true;
}

void IotsaRTMod::doStimulus() {
  if (stimulus == stim_rise) {
    currentOutputHi = true;
  } else if (stimulus == stim_fall) {
    currentOutputHi = false;
  } else {
    currentOutputHi = !currentOutputHi;
  }

  if (response == resp_same) {
    responseWanted = currentOutputHi;
  } else if (response == resp_reverse) {
    responseWanted = !currentOutputHi;
  } else if (response == resp_rise) {
    responseWanted = true;
  } else {
    responseWanted = false;
  }

  noInterrupts();
  responseSeen = false;
  stimMillis = millis();
  stimMicros = micros();
  digitalWrite(outPin, currentOutputHi ? HIGH : LOW);
  if (stimulus != stim_toggle) stimReset = stimMillis + duration;
  interrupts();
}

void IotsaRTMod::loop() {
  bool curInput = digitalRead(inPin) == HIGH;
  noInterrupts();
  if (!responseSeen && curInput == responseWanted) {
    respMicros = micros();
    respMillis = millis();
    responseSeen = true;
  }
  interrupts();
  if (stimReset && millis() > stimReset) {
    stimReset = 0;
    currentOutputHi = !currentOutputHi;
    digitalWrite(outPin, currentOutputHi ? HIGH : LOW);
 }
 if (trigger) {
   trigger = false;
   doStimulus();
 }
}

//
// Conversion of stimulus and response calues to strings
//

static const char *stimNames[] = {"rise", "fall", "toggle", NULL};
static const char *respNames[] = {"same", "reverse", "rise", "fall", NULL};

static const char *stim2str(stimulusType s) {
  if ((unsigned int)s < 3) return stimNames[(unsigned int)s];
  return "";
}

static const char *resp2str(responseType s) {
  if ((unsigned int)s < 3) return respNames[(unsigned int)s];
  return "";
}

static stimulusType str2stim(const char *s) {
  for(const char **p=stimNames; *p; p++) {
    if (strcmp(s, *p) == 0) return (stimulusType)(p-stimNames);
  }
  return stim_rise;
}

static responseType str2resp(const char *s) {
  for(const char **p=respNames; *p; p++) {
    if (strcmp(s, *p) == 0) return (responseType)(p-respNames);
  }
  return resp_same;
}

//
// response time module
//
void
IotsaRTMod::handler() {
  // First check configuration changes
  bool anyChanged = false;
  stimulusType _stimulus = stimulus;
  responseType _response = response;
  int _duration = duration;
  if (server->hasArg("stimulus")) {
    _stimulus = str2stim(server->arg("stimulus").c_str());
    anyChanged = true;
  }
  if (server->hasArg("response")) {
    _response = str2resp(server->arg("response").c_str());
    anyChanged = true;
  }
  if (server->hasArg("duration")) {
    _duration = server->arg("duration").toInt();
    anyChanged = true;
  }
  if (anyChanged && !iotsaConfig.inConfigurationMode()) {
    server->send(401, "text/plain", "401 Unauthorized, not in configuration mode");
    return;
  }

  if (anyChanged) {
    stimulus = _stimulus;
    response = _response;
    duration = _duration;
    configSave();
  }

  String message = "<html><head><title>Response time module</title></head><body><h1>Response time Configuration</h1>";
  message += "<form method='get'>";

  message += "Stimulus signal: <select name='stimulus'>";
  message += "<option value='rise'"; if (stimulus == stim_rise) message += " selected";   message += ">Rise</option>";
  message += "<option value='fall'"; if (stimulus == stim_fall) message += " selected";   message += ">Fall</option>";
  message += "<option value='toggle'"; if (stimulus == stim_toggle) message += " selected";   message += ">Toggle</option>";
  message += "</select>";

  message += "Response signal: <select name='response'>";
  message += "<option value='same'"; if (response == resp_same) message += " selected";   message += ">Same</option>";
  message += "<option value='reverse'"; if (response == resp_reverse) message += " selected";   message += ">Reverse</option>";
  message += "<option value='rise'"; if (response == resp_rise) message += " selected";   message += ">Rise</option>";
  message += "<option value='fall'"; if (response == resp_fall) message += " selected";   message += ">Fall</option>";
  message += "</select>";

  message += "Stimulus reset time (ms): <input name='duration' type='number' value=" + String(duration) + ">";
  
  message += "<input type='submit'></form>";

  server->send(200, "text/html", message);
}

bool IotsaRTMod::getHandler(const char *path, JsonObject& reply) {
  if (strcmp(path, "/api/stimulus") == 0) {
    if (canDoStimulus()) {
      trigger = true;
      return true;
    }
    return false;
  } else if (strcmp(path, "/api/response") == 0) {
    if (responseSeen) {
      reply["millis"] = respMillis - stimMillis;
      reply["micros"] = respMicros - stimMicros;
    }
    return true;
  }
  // Otherwise assume it is config
  reply["stimulus"] = stim2str(stimulus);
  reply["response"] = resp2str(response);
  reply["duration"] = duration;
  return true;
}

bool IotsaRTMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  if (!iotsaConfig.inConfigurationMode()) return false;
  JsonObject args = request.as<JsonObject>();
  bool anyDone = false;
  if (args.containsKey("stimulus")) {
    stimulus = str2stim(args["stimulus"].as<char *>());
    anyDone = true;
  }
  if (args.containsKey("response")) {
    stimulus = str2stim(args["response"].as<char *>());
    anyDone = true;
  }
  if (args.containsKey("duration")) {
    duration = args["duration"].as<int>();
    anyDone = true;
  }

  if (anyDone) configSave();
  return anyDone;
}

void IotsaRTMod::setup() {
  IFDEBUG IotsaSerial.println("xxxjack iotsartmod setup");
  pinMode(outPin, OUTPUT);
  pinMode(inPin, INPUT_PULLUP);
  configLoad();
}

void IotsaRTMod::serverSetup() {
  server->on("/rtconfig", std::bind(&IotsaRTMod::handler, this));
  api.setup("/api/rtconfig", true, true);
  api.setup("/api/stimulus", true);
  api.setup("/api/response", true);
}

void IotsaRTMod::configLoad() {
  IotsaConfigFileLoad cf("/config/RT.cfg");
  String s;
  cf.get("stimulus", s, "rise");
  stimulus = str2stim(s.c_str());
  cf.get("response", s, "same");
  response = str2resp(s.c_str());
  cf.get("duration", duration, 0);
}

void IotsaRTMod::configSave() {
  IotsaConfigFileSave cf("/config/RT.cfg");
  String s = stim2str(stimulus);
  cf.put("stimulus", s);
  s = resp2str(response);
  cf.put("response", s);
  cf.put("duration", duration);
}

String IotsaRTMod::info() {
  String message = "<p>Built with RT module. See <a href=\"/rtconfig\">/rtconfig</a> to examine stimulus/response configuration, ";
  message += "<a href=\"/api/rtconfig\">/api/rtconfig</a> for REST access to configuration, <a href=\"/api/stimulus\">/api/stimulus</a> to trigger a stimulus and <a href=\"/api/response\">/api/response</a> to examine the response time.</p>";
  return message;
}
