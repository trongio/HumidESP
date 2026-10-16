// HumidESP: ESP32 multi-sensor humidity controller with its own Wi-Fi AP and web UI.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <esp_timer.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <deque>
#include <map>
#include <vector>

#include "config.h"
#include "sensors.h"
#include "index_html.h"
#include "ota.h"
#include "pwa.h"

#if __has_include("secrets.h")
#include "secrets.h"  // optional, gitignored: DEFAULT_STA_SSID / DEFAULT_STA_PASS
#endif
#ifndef DEFAULT_STA_SSID
#define DEFAULT_STA_SSID ""
#define DEFAULT_STA_PASS ""
#endif

// ---------------------------------------------------------------------------
// Types and state
// ---------------------------------------------------------------------------
enum Mode : uint8_t { MODE_OFF = 0, MODE_ON = 1, MODE_AUTO = 2 };
static const char *MODE_NAMES[] = {"off", "on", "auto"};
enum AggMode : uint8_t { AGG_AVG = 0, AGG_MIN = 1, AGG_MAX = 2 };
static const char *AGG_NAMES[] = {"avg", "min", "max"};

struct Settings {
  float target = 92;          // apples: 90-95 %RH
  float hyst = 2;
  float alertLow = 85;
  float alertHigh = 98;
  float tempLow = -0.5;       // freezing risk for apples
  float tempHigh = 5;         // cooling failure
  float spreadMax = 8;        // max RH difference between sensors (0 = off)
  uint8_t aggMode = AGG_AVG;
  uint16_t minCycleS = 30;    // min time between auto on/off switches
  uint16_t maxRunMin = 180;   // alert if running continuously longer (0 = off)
  uint16_t noRiseMin = NO_RISE_CHECK_MIN; // 0 = off
  bool noRiseStop = true;     // stop auto channel when no rise (empty tank?)
  bool waterEnabled = false;
  bool waterOkWhenLow = true; // float switch closed (pin LOW) means water OK
  bool buzzer = false;
  bool autoUpdate = true;     // install new GitHub releases by itself
  String webhook;
  String apPass = AP_DEFAULT_PASS;
  String staSsid;
  String staPass;
  uint8_t modes[NUM_CHANNELS];
  String names[NUM_CHANNELS];
  uint8_t presses[NUM_CHANNELS];  // button presses needed after power on
};

struct Layout {
  float w = 6, l = 5, h = 4;  // room size in metres
  float chx[NUM_CHANNELS], chy[NUM_CHANNELS];
};

struct SensorState {
  float rh = NAN, t = NAN;    // filtered + calibrated
  float rbuf[5], tbuf[5];
  int n = 0, i = 0;
  uint32_t lastOk = 0;
  bool everOk = false;
  SensorRuntime rt;
};

struct Channel {
  bool on = false;
  bool lockout = false;     // stopped by no-rise safety until user resumes
  uint32_t lastChange = 0;  // millis
  uint32_t onSince = 0;     // millis, reset whenever RH clearly rises
  float rhMinSinceOn = 0;
  uint8_t pressLeft = 0;    // queued button presses
  bool pressing = false;
  uint32_t pressAt = 0;     // millis of next press edge
};

struct Alert {
  String msg;
};

struct LogEntry {
  uint32_t t;  // uptime seconds
  char msg[100];
};

// Series 0 is the control value, 1..MAX_SENSORS the individual sensors.
static const int SERIES = MAX_SENSORS + 1;
struct HistPoint {
  int16_t rh[SERIES];
  int16_t t[SERIES];
  uint8_t on;  // bit per channel
};

static Settings S;
static Layout L;
static ModbusBus bus;
static std::vector<SensorCfg> sensors;
static SensorState sst[MAX_SENSORS];
static Channel ch[NUM_CHANNELS];
static std::map<String, Alert> alerts;

static const int LOG_LEN = 50;
static LogEntry logBuf[LOG_LEN];
static int logHead = 0, logCount = 0;

static HistPoint *hist = nullptr;
static int histHead = 0, histCount = 0;
static double accRh[SERIES], accT[SERIES];
static int accN[SERIES];
static uint8_t accOn = 0;
static uint32_t lastHistMs = 0;

static float ctlRh = NAN, ctlT = NAN;  // control value (aggregate of control sensors)
static int ctlN = 0;
static int pollIdx = 0;
static uint32_t lastPollMs = 0, lastControlMs = 0;

static std::deque<String> notifyQueue;
static uint32_t lastNotifyMs = 0;
static int beepsLeft = 0;
static uint32_t lastBeepMs = 0;

static uint32_t restartAt = 0, apRestartAt = 0, lastStaAttempt = 0;
static String scanCache = "[]";

// Web password (off while authOn is false) and login sessions, oldest first, all zero = free.
static const int MAX_SESSIONS = 8;
static bool authOn = false, uploadDenied = false;
static uint8_t pwSalt[16], pwHash[32], sessions[MAX_SESSIONS][16];
static int loginFails = 0;
static uint32_t lockUntil = 0, ledFastUntil = 0;

static WebServer server(80);
static DNSServer dns;
static Preferences prefs;
static const IPAddress AP_IP(192, 168, 4, 1);

static uint32_t uptimeS() { return (uint32_t)(esp_timer_get_time() / 1000000ULL); }
static String chKey(const char *p, int i) { return String(p) + i; }

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------
static void sensorsFromJson(JsonArrayConst a) {
  sensors.clear();
  for (JsonObjectConst o : a) {
    if (sensors.size() >= MAX_SENSORS) break;
    int type = sensorTypeFromKey(o["type"] | "");
    if (type < 0) continue;
    SensorCfg c;
    c.type = type;
    c.addr = o["addr"] | 0;
    c.name = o["name"] | String(String("Sensor ") + (sensors.size() + 1));
    c.name = c.name.substring(0, 24);
    c.x = constrain(o["x"] | 0.5f, 0.0f, 1.0f);
    c.y = constrain(o["y"] | 0.5f, 0.0f, 1.0f);
    c.z = constrain(o["z"] | 1.5f, 0.0f, 20.0f);
    c.control = o["control"] | true;
    c.hOff = constrain(o["hOff"] | 0.0f, -20.0f, 20.0f);
    c.tOff = constrain(o["tOff"] | 0.0f, -10.0f, 10.0f);
    sensors.push_back(c);
  }
}

static void sensorsToJson(JsonArray a) {
  for (auto &c : sensors) {
    JsonObject o = a.add<JsonObject>();
    o["type"] = SENSOR_TYPE_KEYS[c.type];
    o["addr"] = c.addr;
    o["name"] = c.name;
    o["x"] = c.x;
    o["y"] = c.y;
    o["z"] = c.z;
    o["control"] = c.control;
    o["hOff"] = c.hOff;
    o["tOff"] = c.tOff;
  }
}

static void loadSettings() {
  prefs.begin("humid", true);
  S.target = prefs.getFloat("target", S.target);
  S.hyst = prefs.getFloat("hyst", S.hyst);
  S.alertLow = prefs.getFloat("aLow", S.alertLow);
  S.alertHigh = prefs.getFloat("aHigh", S.alertHigh);
  S.tempLow = prefs.getFloat("tLow", S.tempLow);
  S.tempHigh = prefs.getFloat("tHigh", S.tempHigh);
  S.spreadMax = prefs.getFloat("spread", S.spreadMax);
  S.aggMode = prefs.getUChar("agg", S.aggMode) % 3;
  S.minCycleS = prefs.getUShort("minCyc", S.minCycleS);
  S.maxRunMin = prefs.getUShort("maxRun", S.maxRunMin);
  S.noRiseMin = prefs.getUShort("noRise", S.noRiseMin);
  S.noRiseStop = prefs.getBool("noRiseStop", S.noRiseStop);
  S.waterEnabled = prefs.getBool("water", false);
  S.waterOkWhenLow = prefs.getBool("waterLow", true);
  S.buzzer = prefs.getBool("buzzer", false);
  S.autoUpdate = prefs.getBool("autoUpd", true);
  S.webhook = prefs.getString("webhook", "");
  S.apPass = prefs.getString("apPass", AP_DEFAULT_PASS);
  S.staSsid = prefs.getString("ssid", DEFAULT_STA_SSID);
  S.staPass = prefs.getString("pass", DEFAULT_STA_PASS);
  for (int i = 0; i < NUM_CHANNELS; i++) {
    S.modes[i] = prefs.getUChar(chKey("mode", i).c_str(), MODE_AUTO) % 3;
    S.names[i] = prefs.getString(chKey("name", i).c_str(), "Humidifier " + String(i + 1));
    S.presses[i] = min<uint8_t>(prefs.getUChar(chKey("press", i).c_str(), 0), 5);
    L.chx[i] = 0.1f + 0.8f * i / max(1, NUM_CHANNELS - 1);
    L.chy[i] = 0.9f;
  }
  bus.baud = prefs.getUInt("baud", bus.baud);
  bus.fc = prefs.getUChar("mbFc", bus.fc);
  bus.humReg = prefs.getUShort("mbHum", bus.humReg);
  bus.tempReg = prefs.getUShort("mbTemp", bus.tempReg);
  bus.addrReg = prefs.getUShort("mbAddr", bus.addrReg);

  JsonDocument d;
  if (!deserializeJson(d, prefs.getString("layout", "{}"))) {
    L.w = d["w"] | L.w;
    L.l = d["l"] | L.l;
    L.h = d["h"] | L.h;
    JsonArray a = d["ch"];
    for (int i = 0; i < NUM_CHANNELS && i < (int)a.size(); i++) {
      L.chx[i] = a[i]["x"] | L.chx[i];
      L.chy[i] = a[i]["y"] | L.chy[i];
    }
  }
  d.clear();
  String sj = prefs.getString("sensors", "");
  if (sj.length() && !deserializeJson(d, sj)) sensorsFromJson(d.as<JsonArrayConst>());
  if (prefs.isKey("pwHash"))
    authOn = prefs.getBytes("pwHash", pwHash, 32) == 32 && prefs.getBytes("pwSalt", pwSalt, 16) == 16;
  if (prefs.isKey("sess")) prefs.getBytes("sess", sessions, sizeof(sessions));
  prefs.end();

  if (sensors.empty()) {  // first boot: the DHT11 on the desk
    SensorCfg c;
    c.type = ST_DHT11;
    c.name = "Desk DHT11";
    sensors.push_back(c);
  }
}

static void saveSettings() {
  prefs.begin("humid", false);
  prefs.putFloat("target", S.target);
  prefs.putFloat("hyst", S.hyst);
  prefs.putFloat("aLow", S.alertLow);
  prefs.putFloat("aHigh", S.alertHigh);
  prefs.putFloat("tLow", S.tempLow);
  prefs.putFloat("tHigh", S.tempHigh);
  prefs.putFloat("spread", S.spreadMax);
  prefs.putUChar("agg", S.aggMode);
  prefs.putUShort("minCyc", S.minCycleS);
  prefs.putUShort("maxRun", S.maxRunMin);
  prefs.putUShort("noRise", S.noRiseMin);
  prefs.putBool("noRiseStop", S.noRiseStop);
  prefs.putBool("water", S.waterEnabled);
  prefs.putBool("waterLow", S.waterOkWhenLow);
  prefs.putBool("buzzer", S.buzzer);
  prefs.putBool("autoUpd", S.autoUpdate);
  prefs.putString("webhook", S.webhook);
  prefs.putString("apPass", S.apPass);
  prefs.putString("ssid", S.staSsid);
  prefs.putString("pass", S.staPass);
  for (int i = 0; i < NUM_CHANNELS; i++) {
    prefs.putUChar(chKey("mode", i).c_str(), S.modes[i]);
    prefs.putString(chKey("name", i).c_str(), S.names[i]);
    prefs.putUChar(chKey("press", i).c_str(), S.presses[i]);
  }
  prefs.putUInt("baud", bus.baud);
  prefs.putUChar("mbFc", bus.fc);
  prefs.putUShort("mbHum", bus.humReg);
  prefs.putUShort("mbTemp", bus.tempReg);
  prefs.putUShort("mbAddr", bus.addrReg);

  JsonDocument d;
  d["w"] = L.w;
  d["l"] = L.l;
  d["h"] = L.h;
  JsonArray a = d["ch"].to<JsonArray>();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    JsonObject o = a.add<JsonObject>();
    o["x"] = L.chx[i];
    o["y"] = L.chy[i];
  }
  String out;
  serializeJson(d, out);
  prefs.putString("layout", out);

  JsonDocument sd;
  sensorsToJson(sd.to<JsonArray>());
  out = "";
  serializeJson(sd, out);
  prefs.putString("sensors", out);
  prefs.end();
}

// ---------------------------------------------------------------------------
// Logging, alerts, notifications
// ---------------------------------------------------------------------------
static void logEvent(const String &msg) {
  Serial.printf("[%lu] %s\n", (unsigned long)uptimeS(), msg.c_str());
  LogEntry &e = logBuf[logHead];
  e.t = uptimeS();
  strlcpy(e.msg, msg.c_str(), sizeof(e.msg));
  logHead = (logHead + 1) % LOG_LEN;
  if (logCount < LOG_LEN) logCount++;
}

static void notify(const String &msg) {
  if (S.webhook.isEmpty()) return;
  if (notifyQueue.size() >= 10) notifyQueue.pop_front();
  notifyQueue.push_back(msg);
}

static void setAlert(const String &key, bool active, const String &msg = "") {
  auto it = alerts.find(key);
  if (active && it == alerts.end()) {
    alerts[key] = {msg};
    logEvent("ALERT: " + msg);
    notify(msg);
    if (S.buzzer) beepsLeft = 6;  // 3 beeps
  } else if (!active && it != alerts.end()) {
    logEvent("Cleared: " + it->second.msg);
    notify("OK: " + it->second.msg + " (cleared)");
    alerts.erase(it);
  } else if (active) {
    it->second.msg = msg;  // keep the value in the message fresh
  }
}

static void processNotifications() {
  if (notifyQueue.empty() || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastNotifyMs < 2000) return;
  lastNotifyMs = millis();
  String msg = notifyQueue.front();
  notifyQueue.pop_front();

  WiFiClientSecure secure;
  WiFiClient plain;
  HTTPClient http;
  bool ok;
  if (S.webhook.startsWith("https://")) {
    secure.setInsecure();
    ok = http.begin(secure, S.webhook);
  } else {
    ok = http.begin(plain, S.webhook);
  }
  if (!ok) return;
  http.setTimeout(5000);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Title", "HumidESP");
  http.addHeader("Tags", "droplet");
  int code = http.POST(msg);
  http.end();
  Serial.printf("notify -> %d\n", code);
}

// ---------------------------------------------------------------------------
// Sensors
// ---------------------------------------------------------------------------
static float median(const float *v, int n) {
  float t[5];
  memcpy(t, v, n * sizeof(float));
  std::sort(t, t + n);
  return n % 2 ? t[n / 2] : (t[n / 2 - 1] + t[n / 2]) / 2;
}

static bool sensorOk(int i) { return sst[i].everOk && millis() - sst[i].lastOk < SENSOR_STALE_MS; }

static void resetSensors() {
  for (auto &s : sst) s = SensorState();
  pollIdx = 0;
  // Old history columns belong to the old sensor list.
  if (hist) memset(hist, 0, sizeof(HistPoint) * HISTORY_LEN);
  histHead = histCount = 0;
  memset(accN, 0, sizeof(accN));
  memset(accRh, 0, sizeof(accRh));
  memset(accT, 0, sizeof(accT));
  for (auto it = alerts.begin(); it != alerts.end();)
    it = it->first.startsWith("sensor") ? alerts.erase(it) : std::next(it);
}

static void pollSensor() {
  if (sensors.empty()) return;
  pollIdx %= sensors.size();
  int i = pollIdx++;
  SensorCfg &c = sensors[i];
  SensorState &s = sst[i];
  float h, t;
  if (!sensorRead(c, s.rt, bus, h, t)) return;
  s.rbuf[s.i] = h;
  s.tbuf[s.i] = t;
  s.i = (s.i + 1) % 5;
  if (s.n < 5) s.n++;
  s.rh = constrain(median(s.rbuf, s.n) + c.hOff, 0.0f, 100.0f);
  s.t = median(s.tbuf, s.n) + c.tOff;
  s.lastOk = millis();
  s.everOk = true;
}

static void computeAggregate() {
  float sumH = 0, sumT = 0, mn = 1e9, mx = -1e9, tAtMn = NAN, tAtMx = NAN;
  int n = 0;
  for (size_t i = 0; i < sensors.size(); i++) {
    if (!sensors[i].control || !sensorOk(i)) continue;
    float h = sst[i].rh;
    sumH += h;
    sumT += sst[i].t;
    if (h < mn) { mn = h; tAtMn = sst[i].t; }
    if (h > mx) { mx = h; tAtMx = sst[i].t; }
    n++;
  }
  ctlN = n;
  if (!n) { ctlRh = ctlT = NAN; return; }
  switch (S.aggMode) {
    case AGG_MIN: ctlRh = mn; ctlT = tAtMn; break;
    case AGG_MAX: ctlRh = mx; ctlT = tAtMx; break;
    default: ctlRh = sumH / n; ctlT = sumT / n; break;
  }
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------
static bool waterLow() {
  if (!S.waterEnabled) return false;
  bool pinLow = digitalRead(PIN_WATER) == LOW;
  return S.waterOkWhenLow ? !pinLow : pinLow;
}

static void writeRelay(int i, bool on) {
  digitalWrite(RELAY_PINS[i], (on == RELAY_ACTIVE_HIGH) ? HIGH : LOW);
}

static void setChannel(int i, bool on, const char *why) {
  Channel &c = ch[i];
  if (c.on == on) return;
  c.on = on;
  c.lastChange = millis();
  if (on) {
    c.onSince = millis();
    c.rhMinSinceOn = isnan(ctlRh) ? 0 : ctlRh;
  }
  writeRelay(i, on);
  c.pressing = false;
  digitalWrite(BUTTON_PINS[i], LOW);
  c.pressLeft = on ? S.presses[i] : 0;
  c.pressAt = millis() + BUTTON_DELAY_MS;
  logEvent(S.names[i] + (on ? " ON" : " OFF") + " (" + why + ")");
}

static void sensorAlerts() {
  float tMin = 1e9, tMax = -1e9, hMin = 1e9, hMax = -1e9;
  String tMinName, tMaxName, hMinName, hMaxName;
  int okCount = 0;
  for (size_t i = 0; i < sensors.size(); i++) {
    bool ok = sensorOk(i);
    // Sensors that never answered since boot also count as offline after the stale window.
    bool offline = !ok && (sst[i].everOk || millis() > SENSOR_STALE_MS);
    setAlert("sensor" + String(i), offline, "Sensor \"" + sensors[i].name + "\" offline");
    if (!ok) continue;
    okCount++;
    float t = sst[i].t, h = sst[i].rh;
    if (t < tMin) { tMin = t; tMinName = sensors[i].name; }
    if (t > tMax) { tMax = t; tMaxName = sensors[i].name; }
    if (h < hMin) { hMin = h; hMinName = sensors[i].name; }
    if (h > hMax) { hMax = h; hMaxName = sensors[i].name; }
  }
  if (!okCount) return;
  if (tMin < S.tempLow)
    setAlert("tlow", true, "Too cold at " + tMinName + ": " + String(tMin, 1) + " C (alarm below " + String(S.tempLow, 1) + ")");
  else if (tMin >= S.tempLow + 0.5)
    setAlert("tlow", false);
  if (tMax > S.tempHigh)
    setAlert("thigh", true, "Too warm at " + tMaxName + ": " + String(tMax, 1) + " C (alarm above " + String(S.tempHigh, 1) + ")");
  else if (tMax <= S.tempHigh - 0.5)
    setAlert("thigh", false);
  float spread = hMax - hMin;
  if (S.spreadMax > 0 && okCount > 1 && spread > S.spreadMax)
    setAlert("spread", true, "Uneven humidity: " + hMinName + " " + String(hMin, 0) + "% vs " + hMaxName + " " +
                                 String(hMax, 0) + "%. Improve air circulation or move humidifier");
  else if (spread <= S.spreadMax - 1 || okCount < 2)
    setAlert("spread", false);
}

static void control() {
  computeAggregate();
  sensorAlerts();
  bool haveValue = !isnan(ctlRh);
  bool lowWater = waterLow();
  setAlert("nosensor", !haveValue && millis() > SENSOR_STALE_MS,
           "No working control sensor: auto humidifiers stopped");
  setAlert("water", lowWater, "Water tank low: humidifiers stopped");

  if (haveValue) {
    if (S.alertLow > 0 && ctlRh < S.alertLow)
      setAlert("low", true, "Humidity low: " + String(ctlRh, 0) + "% (alarm below " + String(S.alertLow, 0) + "%)");
    else if (ctlRh >= S.alertLow + 1)
      setAlert("low", false);
    if (S.alertHigh < 100 && ctlRh > S.alertHigh)
      setAlert("high", true, "Humidity high: " + String(ctlRh, 0) + "% (alarm above " + String(S.alertHigh, 0) + "%)");
    else if (ctlRh <= S.alertHigh - 1)
      setAlert("high", false);
  }

  bool anyNoRise = false, anyLongRun = false;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Channel &c = ch[i];
    uint8_t mode = S.modes[i];
    bool want = false;
    const char *why = MODE_NAMES[mode];

    if (mode == MODE_ON) {
      want = true;
    } else if (mode == MODE_AUTO) {
      if (!haveValue) { want = false; why = "no sensor"; }
      else if (c.lockout) { want = false; why = "safety stop"; }
      else if (c.on) { want = ctlRh < S.target; why = want ? "auto" : "target reached"; }
      else { want = ctlRh <= S.target - S.hyst; why = "below target"; }
    }
    if (lowWater) { want = false; why = "water low"; }

    if (want != c.on) {
      // Auto switching respects the min cycle time to save the relay.
      bool forced = mode != MODE_AUTO || lowWater || !haveValue || c.lockout;
      if (forced || millis() - c.lastChange >= (uint32_t)S.minCycleS * 1000) setChannel(i, want, why);
    }

    if (c.on) {
      if (haveValue) {
        if (ctlRh < c.rhMinSinceOn) c.rhMinSinceOn = ctlRh;
        // Progress resets the no-rise timer, so slow but steady humidifying is fine.
        if (ctlRh >= c.rhMinSinceOn + 1) { c.rhMinSinceOn = ctlRh; c.onSince = millis(); }
      }
      if (S.maxRunMin && (millis() - c.lastChange) / 60000 >= S.maxRunMin) anyLongRun = true;
      if (mode == MODE_AUTO && S.noRiseMin && haveValue && (millis() - c.onSince) / 60000 >= S.noRiseMin && S.noRiseStop) {
        c.lockout = true;
        setChannel(i, false, "no humidity rise");
      } else if (mode == MODE_AUTO && S.noRiseMin && haveValue && (millis() - c.onSince) / 60000 >= S.noRiseMin) {
        anyNoRise = true;
      }
    }
    if (c.lockout) anyNoRise = true;
  }
  setAlert("norise", anyNoRise,
           S.noRiseStop ? "No humidity rise while running: tank empty? Auto stopped, press Resume"
                        : "No humidity rise while running: tank empty?");
  setAlert("longrun", anyLongRun, "Humidifier running over " + String(S.maxRunMin) + " min without reaching target");
}

static int16_t enc(double sum, int n) { return n ? (int16_t)lround(sum / n * 10) : INT16_MIN; }

static void recordHistory() {
  if (!hist) return;
  if (!isnan(ctlRh)) { accRh[0] += ctlRh; accT[0] += ctlT; accN[0]++; }
  for (size_t i = 0; i < sensors.size(); i++)
    if (sensorOk(i)) { accRh[i + 1] += sst[i].rh; accT[i + 1] += sst[i].t; accN[i + 1]++; }
  for (int i = 0; i < NUM_CHANNELS; i++) if (ch[i].on) accOn |= 1 << i;
  if (millis() - lastHistMs < HISTORY_INTERVAL_S * 1000UL) return;
  lastHistMs = millis();
  HistPoint &p = hist[histHead];
  for (int k = 0; k < SERIES; k++) {
    p.rh[k] = enc(accRh[k], accN[k]);
    p.t[k] = enc(accT[k], accN[k]);
    accRh[k] = accT[k] = 0;
    accN[k] = 0;
  }
  p.on = accOn;
  accOn = 0;
  histHead = (histHead + 1) % HISTORY_LEN;
  if (histCount < HISTORY_LEN) histCount++;
}

// Non-blocking button presses through the optocouplers.
static void serviceButtons() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Channel &c = ch[i];
    if (!c.pressLeft || (int32_t)(millis() - c.pressAt) < 0) continue;
    c.pressing = !c.pressing;
    digitalWrite(BUTTON_PINS[i], c.pressing);
    if (c.pressing) c.pressAt = millis() + BUTTON_PRESS_MS;
    else { c.pressLeft--; c.pressAt = millis() + BUTTON_GAP_MS; }
  }
}

static void updateIndicators() {
  bool any = false;
  for (auto &c : ch) any |= c.on;
  if (ledFastUntil && (int32_t)(millis() - ledFastUntil) >= 0) ledFastUntil = 0;
  if (ledFastUntil) digitalWrite(PIN_LED, (millis() / 60) % 2);  // BOOT button reset acknowledged
  else if (!alerts.empty()) digitalWrite(PIN_LED, (millis() / 400) % 2);
  else digitalWrite(PIN_LED, any);

  if (beepsLeft > 0 && millis() - lastBeepMs > 150) {
    lastBeepMs = millis();
    beepsLeft--;
    digitalWrite(PIN_BUZZER, (beepsLeft % 2) == BUZZER_ACTIVE_HIGH);
  } else if (beepsLeft == 0) {
    digitalWrite(PIN_BUZZER, !BUZZER_ACTIVE_HIGH);
  }
}

// ---------------------------------------------------------------------------
// Wi-Fi
// ---------------------------------------------------------------------------
static void startAP() {
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, S.apPass.c_str());
}

static void startSTA() {
  if (S.staSsid.isEmpty()) return;
  lastStaAttempt = millis();
  WiFi.begin(S.staSsid.c_str(), S.staPass.c_str());
}

static void maintainWifi() {
  if (S.staSsid.isEmpty() || WiFi.status() == WL_CONNECTED) return;
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
  // Retrying makes the radio hop channels and briefly disturbs the AP, so keep it slow.
  if (millis() - lastStaAttempt > 60000) {
    WiFi.disconnect(false);
    startSTA();
  }
}

// ---------------------------------------------------------------------------
// HTTP API
// ---------------------------------------------------------------------------
static void sendJson(JsonDocument &doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json", out);
}

static void sendOk() { server.send(200, "application/json", "{\"ok\":true}"); }
static void sendErr(const char *msg) { server.send(400, "application/json", String("{\"error\":\"") + msg + "\"}"); }

static bool parseBody(JsonDocument &doc) {
  if (deserializeJson(doc, server.arg("plain"))) {
    sendErr("bad json");
    return false;
  }
  return true;
}

// ---------- Web password and sessions ----------
static bool ctEqual(const uint8_t *a, const uint8_t *b, size_t n) {  // constant time
  uint8_t diff = 0;
  for (size_t i = 0; i < n; i++) diff |= a[i] ^ b[i];
  return !diff;
}

static void randomFill(uint8_t *b) {  // 16 bytes from the hardware RNG
  for (int i = 0; i < 16; i += 4) { uint32_t r = esp_random(); memcpy(b + i, &r, 4); }
}

static void hashPassword(const char *pw, uint8_t *out) {  // SHA-256(salt + password)
  mbedtls_sha256_context c;
  mbedtls_sha256_init(&c);
  mbedtls_sha256_starts_ret(&c, 0);
  mbedtls_sha256_update_ret(&c, pwSalt, sizeof(pwSalt));
  mbedtls_sha256_update_ret(&c, (const uint8_t *)pw, strlen(pw));
  mbedtls_sha256_finish_ret(&c, out);
  mbedtls_sha256_free(&c);
}

static void saveAuth() {
  prefs.begin("humid", false);
  if (authOn) {
    prefs.putBytes("pwSalt", pwSalt, sizeof(pwSalt));
    prefs.putBytes("pwHash", pwHash, sizeof(pwHash));
  } else if (prefs.isKey("pwHash")) {
    prefs.remove("pwHash");
    prefs.remove("pwSalt");
  }
  prefs.putBytes("sess", sessions, sizeof(sessions));
  prefs.end();
}

static void clearAuth() {  // password off, all sessions gone
  authOn = false;
  memset(sessions, 0, sizeof(sessions));
  loginFails = lockUntil = 0;
  saveAuth();
}

// Session slot named by the request's hsid cookie, or -1. Every slot is compared.
static int currentSession() {
  String c = server.header("Cookie");
  int p = -1;
  do p = c.indexOf("hsid=", p + 1); while (p > 0 && c[p - 1] != ' ' && c[p - 1] != ';');
  if (p < 0 || (int)c.length() < p + 37) return -1;
  uint8_t tok[16], zero[16] = {0};
  for (int i = 0; i < 32; i++) {
    char h = c[p + 5 + i];
    int v = isdigit(h) ? h - '0' : (h >= 'a' && h <= 'f') ? h - 'a' + 10 : -1;
    if (v < 0) return -1;
    tok[i / 2] = (i % 2) ? (tok[i / 2] | v) : v << 4;
  }
  int found = -1;
  for (int i = 0; i < MAX_SESSIONS; i++)
    if (ctEqual(sessions[i], tok, 16) & !ctEqual(tok, zero, 16)) found = i;
  return found;
}

static bool authorized() { return !authOn || currentSession() >= 0; }
static void sendLoginRequired() { server.send(401, "application/json", "{\"error\":\"login required\"}"); }

static void setCookie(const String &tok, int maxAge) {
  server.sendHeader("Set-Cookie", "hsid=" + tok + "; Path=/; Max-Age=" + maxAge + "; HttpOnly; SameSite=Strict");
}

static void startSession() {  // new random token in the newest slot, evicting the oldest when full
  int n = 0;
  uint8_t zero[16] = {0};
  while (n < MAX_SESSIONS && !ctEqual(sessions[n], zero, 16)) n++;
  if (n == MAX_SESSIONS) memmove(sessions[0], sessions[1], sizeof(sessions) - 16), n--;
  char hex[33];
  randomFill(sessions[n]);
  for (int i = 0; i < 16; i++) sprintf(hex + 2 * i, "%02x", sessions[n][i]);
  saveAuth();
  setCookie(hex, 2592000);
}

// Checks a password with brute-force lockout. On failure it sends 403/429 and returns false.
static bool checkPassword(const char *pw) {
  if (lockUntil && (int32_t)(millis() - lockUntil) >= 0) lockUntil = 0;
  if (!lockUntil) {
    uint8_t h[32];
    hashPassword(pw, h);
    if (ctEqual(h, pwHash, 32)) { loginFails = 0; return true; }
    loginFails = min(loginFails + 1, 100);
    if (loginFails < 5) { server.send(403, "application/json", "{\"error\":\"wrong password\"}"); return false; }
    uint32_t secs = min<uint32_t>(30UL << min(loginFails - 5, 5), 600);  // 30 s doubling up to 10 min
    lockUntil = (millis() + secs * 1000) | 1;
    logEvent("Too many wrong web passwords, login locked for " + String(secs) + " s");
  }
  String s((lockUntil - millis()) / 1000 + 1);
  server.sendHeader("Retry-After", s);
  server.send(429, "application/json", "{\"error\":\"too many attempts, try again in " + s + " s\",\"retryAfter\":" + s + "}");
  return false;
}

static void handleAuth() {
  server.send(200, "application/json", String("{\"enabled\":") + (authOn ? "true" : "false") +
                                           ",\"loggedIn\":" + (authorized() ? "true" : "false") + "}");
}

static void handleLogin() {
  JsonDocument d;
  if (!parseBody(d)) return;
  if (authOn && !checkPassword(d["password"] | "")) return;
  if (authOn) startSession();
  sendOk();
}

static void handleLogout() {
  int i = currentSession();
  if (i >= 0) {
    memmove(sessions[i], sessions[i + 1], (MAX_SESSIONS - 1 - i) * 16);
    memset(sessions[MAX_SESSIONS - 1], 0, 16);
    saveAuth();
  }
  setCookie("", 0);
  sendOk();
}

// {"current", "password"} sets or changes, {"current", "remove":true} turns protection off.
static void handlePassword() {
  JsonDocument d;
  if (!parseBody(d)) return;
  bool remove = d["remove"] | false;
  const char *pw = d["password"] | "";
  if (!remove && (strlen(pw) < 4 || strlen(pw) > 64)) return sendErr("Web password must be 4-64 characters");
  if (authOn && !checkPassword(d["current"] | "")) return;
  bool was = authOn;
  clearAuth();  // ends every session
  if (remove) {
    setCookie("", 0);
    logEvent("Web password removed");
  } else {
    randomFill(pwSalt);
    hashPassword(pw, pwHash);
    authOn = true;
    startSession();  // keeps this browser logged in (also saves the hash)
    logEvent(was ? "Web password changed" : "Web password set");
  }
  sendOk();
}

// Recovery: hold BOOT for 10 s while running. Armed only after the pin reads high once,
// so a button or auto-reset line still low from flashing can't trigger it.
static void checkBootButton() {
  static bool armed = false, fired = false;
  static uint32_t downAt = 0;
  if (digitalRead(PIN_BOOT_BUTTON) == HIGH) { armed = true; fired = false; downAt = 0; return; }
  if (!armed || fired) return;
  if (!downAt) downAt = millis() | 1;
  if (millis() - downAt < 10000) return;
  fired = true;
  clearAuth();
  logEvent("BOOT button held 10 s: web password and sessions cleared");
  ledFastUntil = (millis() + 2000) | 1;
}

static void handleState() {
  JsonDocument d;
  d["uptime"] = uptimeS();
  d["fw"] = FW_VERSION;
  if (!isnan(ctlRh)) { d["rh"] = ctlRh; d["temp"] = ctlT; }
  d["ctlN"] = ctlN;
  d["water"] = S.waterEnabled ? (waterLow() ? "low" : "ok") : "n/a";

  JsonArray sa = d["sensors"].to<JsonArray>();
  sensorsToJson(sa);
  for (size_t i = 0; i < sensors.size(); i++) {
    JsonObject o = sa[i];
    bool ok = sensorOk(i);
    o["ok"] = ok;
    if (ok) { o["rh"] = sst[i].rh; o["t"] = sst[i].t; }
    if (sst[i].everOk) o["age"] = (millis() - sst[i].lastOk) / 1000;
  }

  JsonArray chs = d["channels"].to<JsonArray>();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    JsonObject o = chs.add<JsonObject>();
    o["name"] = S.names[i];
    o["mode"] = MODE_NAMES[S.modes[i]];
    o["on"] = ch[i].on;
    o["lockout"] = ch[i].lockout;
    o["since"] = (millis() - ch[i].lastChange) / 1000;
    o["x"] = L.chx[i];
    o["y"] = L.chy[i];
    o["presses"] = S.presses[i];
  }

  JsonArray al = d["alerts"].to<JsonArray>();
  for (auto &kv : alerts) {
    JsonObject o = al.add<JsonObject>();
    o["id"] = kv.first;
    o["msg"] = kv.second.msg;
  }

  JsonObject s = d["settings"].to<JsonObject>();
  s["target"] = S.target;
  s["hyst"] = S.hyst;
  s["alertLow"] = S.alertLow;
  s["alertHigh"] = S.alertHigh;
  s["tempLow"] = S.tempLow;
  s["tempHigh"] = S.tempHigh;
  s["spreadMax"] = S.spreadMax;
  s["aggMode"] = AGG_NAMES[S.aggMode];
  s["minCycleS"] = S.minCycleS;
  s["maxRunMin"] = S.maxRunMin;
  s["noRiseMin"] = S.noRiseMin;
  s["noRiseStop"] = S.noRiseStop;
  s["waterEnabled"] = S.waterEnabled;
  s["waterOkWhenLow"] = S.waterOkWhenLow;
  s["buzzer"] = S.buzzer;
  s["autoUpdate"] = S.autoUpdate;
  s["webhook"] = S.webhook;

  JsonObject r = d["room"].to<JsonObject>();
  r["w"] = L.w;
  r["l"] = L.l;
  r["h"] = L.h;

  JsonObject b = d["bus"].to<JsonObject>();
  b["baud"] = bus.baud;
  b["fc"] = bus.fc;
  b["humReg"] = bus.humReg;
  b["tempReg"] = bus.tempReg;
  b["addrReg"] = bus.addrReg;

  JsonObject w = d["wifi"].to<JsonObject>();
  w["apSsid"] = AP_SSID;
  w["apIp"] = WiFi.softAPIP().toString();
  w["apClients"] = WiFi.softAPgetStationNum();
  w["staSsid"] = S.staSsid;
  w["staConnected"] = WiFi.status() == WL_CONNECTED;
  if (WiFi.status() == WL_CONNECTED) {
    w["staIp"] = WiFi.localIP().toString();
    w["rssi"] = WiFi.RSSI();
  }
  w["mdns"] = String(MDNS_HOST) + ".local";
  d["auth"]["enabled"] = authOn;
  if (ota.checkedS) {
    JsonObject o = d["ota"].to<JsonObject>();
    o["latest"] = ota.latest;
    o["available"] = ota.available;
    o["error"] = ota.error;
  }
  sendJson(d);
}

// {"interval":60,"age":s,"on":[..],"rh":[[ctl..],[s1..],..],"t":[[..],..]}, oldest first, null = gap
static void handleHistory() {
  int n = server.hasArg("n") ? constrain(server.arg("n").toInt(), 1, histCount) : histCount;
  int series = 1 + sensors.size();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  int start = (histHead - n + HISTORY_LEN) % HISTORY_LEN;
  String buf;
  buf.reserve(1600);
  buf = "{\"interval\":" + String(HISTORY_INTERVAL_S) + ",\"age\":" + String((millis() - lastHistMs) / 1000) + ",\"on\":[";
  auto flush = [&] { if (buf.length() > 1400) { server.sendContent(buf); buf = ""; } };
  for (int j = 0; j < n; j++) {
    if (j) buf += ',';
    buf += hist ? hist[(start + j) % HISTORY_LEN].on : 0;
    flush();
  }
  buf += "]";
  for (int kind = 0; kind < 2; kind++) {
    buf += kind ? ",\"t\":[" : ",\"rh\":[";
    for (int k = 0; k < series; k++) {
      buf += k ? ",[" : "[";
      for (int j = 0; j < n; j++) {
        const HistPoint &p = hist[(start + j) % HISTORY_LEN];
        int16_t v = kind ? p.t[k] : p.rh[k];
        if (j) buf += ',';
        if (v == INT16_MIN) buf += "null";
        else buf += String(v / 10.0f, 1);
        flush();
      }
      buf += "]";
    }
    buf += "]";
  }
  buf += "}";
  server.sendContent(buf);
  server.sendContent("");
}

static void handleLog() {
  JsonDocument d;
  d["now"] = uptimeS();
  JsonArray a = d["log"].to<JsonArray>();
  for (int n = 0; n < logCount; n++) {  // newest first
    const LogEntry &e = logBuf[(logHead - 1 - n + LOG_LEN) % LOG_LEN];
    JsonObject o = a.add<JsonObject>();
    o["t"] = e.t;
    o["msg"] = e.msg;
  }
  sendJson(d);
}

static void handleSettings() {
  JsonDocument d;
  if (!parseBody(d)) return;
  auto num = [&](const char *k, float &dst, float lo, float hi) {
    if (d[k].is<float>()) dst = constrain(d[k].as<float>(), lo, hi);
  };
  auto u16 = [&](const char *k, uint16_t &dst, int lo, int hi) {
    if (d[k].is<int>()) dst = constrain(d[k].as<int>(), lo, hi);
  };
  auto flag = [&](const char *k, bool &dst) {
    if (d[k].is<bool>()) dst = d[k];
  };
  num("target", S.target, 20, 99);
  num("hyst", S.hyst, 0.5, 15);
  num("alertLow", S.alertLow, 0, 100);
  num("alertHigh", S.alertHigh, 0, 100);
  num("tempLow", S.tempLow, -30, 40);
  num("tempHigh", S.tempHigh, -30, 50);
  num("spreadMax", S.spreadMax, 0, 50);
  u16("minCycleS", S.minCycleS, 5, 3600);
  u16("maxRunMin", S.maxRunMin, 0, 1440);
  u16("noRiseMin", S.noRiseMin, 0, 240);
  flag("noRiseStop", S.noRiseStop);
  flag("waterEnabled", S.waterEnabled);
  flag("waterOkWhenLow", S.waterOkWhenLow);
  flag("buzzer", S.buzzer);
  flag("autoUpdate", S.autoUpdate);
  if (d["aggMode"].is<const char *>())
    for (int k = 0; k < 3; k++) if (d["aggMode"] == AGG_NAMES[k]) S.aggMode = k;
  if (d["webhook"].is<const char *>()) S.webhook = d["webhook"].as<String>();
  if (d["names"].is<JsonArray>()) {
    JsonArray n = d["names"];
    for (int i = 0; i < NUM_CHANNELS && i < (int)n.size(); i++) {
      String v = n[i].as<String>();
      v.trim();
      if (v.length() && v.length() <= 24) S.names[i] = v;
    }
  }
  if (d["presses"].is<JsonArray>()) {
    JsonArray n = d["presses"];
    for (int i = 0; i < NUM_CHANNELS && i < (int)n.size(); i++) S.presses[i] = constrain(n[i].as<int>(), 0, 5);
  }
  if (d["apPass"].is<const char *>()) {
    String p = d["apPass"].as<String>();
    if (p.length() < 8 || p.length() > 63) return sendErr("Hotspot password must be 8-63 characters");
    S.apPass = p;
    apRestartAt = millis() + 1500;
  }
  if (S.alertHigh <= S.alertLow) S.alertHigh = min(100.0f, S.alertLow + 5);
  if (S.tempHigh <= S.tempLow) S.tempHigh = S.tempLow + 1;
  saveSettings();
  control();
  sendOk();
}

// Full replacement of the sensor list plus room layout.
static void handleSensors() {
  JsonDocument d;
  if (!parseBody(d)) return;
  if (d["sensors"].is<JsonArray>()) {
    std::vector<SensorCfg> old = sensors;
    sensorsFromJson(d["sensors"].as<JsonArrayConst>());
    bool changed = old.size() != sensors.size();
    for (size_t i = 0; !changed && i < old.size(); i++)
      changed = old[i].type != sensors[i].type || old[i].addr != sensors[i].addr;
    if (changed) {
      resetSensors();
      logEvent("Sensor list changed (" + String(sensors.size()) + " sensors), history cleared");
    }
  }
  if (d["room"].is<JsonObject>()) {
    L.w = constrain(d["room"]["w"] | L.w, 1.0f, 100.0f);
    L.l = constrain(d["room"]["l"] | L.l, 1.0f, 100.0f);
    L.h = constrain(d["room"]["h"] | L.h, 1.0f, 20.0f);
  }
  if (d["channels"].is<JsonArray>()) {
    JsonArray a = d["channels"];
    for (int i = 0; i < NUM_CHANNELS && i < (int)a.size(); i++) {
      L.chx[i] = constrain(a[i]["x"] | L.chx[i], 0.0f, 1.0f);
      L.chy[i] = constrain(a[i]["y"] | L.chy[i], 0.0f, 1.0f);
    }
  }
  saveSettings();
  sendOk();
}

static void handleBus() {
  JsonDocument d;
  if (!parseBody(d)) return;
  uint32_t baud = d["baud"] | bus.baud;
  static const uint32_t ok[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
  bool valid = false;
  for (auto b : ok) valid |= b == baud;
  if (!valid) return sendErr("unsupported baud rate");
  bus.baud = baud;
  bus.fc = (d["fc"] | bus.fc) == 4 ? 4 : 3;
  bus.humReg = d["humReg"] | bus.humReg;
  bus.tempReg = d["tempReg"] | bus.tempReg;
  bus.addrReg = d["addrReg"] | bus.addrReg;
  Serial2.updateBaudRate(bus.baud);
  saveSettings();
  sendOk();
}

static void handleModbusScan() {
  JsonDocument d;
  if (!parseBody(d)) return;
  int type = sensorTypeFromKey(d["type"] | "xymd02");
  if (type < 0 || !sensorIsModbus(type)) return sendErr("pick a Modbus sensor type");
  int from = constrain(d["from"] | 1, 1, 247), to = constrain(d["to"] | 16, from, min(from + 31, 247));
  JsonDocument r;
  JsonArray found = r["found"].to<JsonArray>();
  SensorCfg c;
  c.type = type;
  SensorRuntime rt;
  for (int a = from; a <= to; a++) {
    c.addr = a;
    float h, t;
    if (sensorRead(c, rt, bus, h, t)) {
      JsonObject o = found.add<JsonObject>();
      o["addr"] = a;
      o["rh"] = h;
      o["t"] = t;
    }
  }
  sendJson(r);
}

static void handleModbusSetAddr() {
  JsonDocument d;
  if (!parseBody(d)) return;
  int type = sensorTypeFromKey(d["type"] | "");
  int from = d["from"] | 0, to = d["to"] | 0;
  if (type < 0 || !sensorIsModbus(type)) return sendErr("pick a Modbus sensor type");
  if (from < 1 || from > 247 || to < 1 || to > 247) return sendErr("addresses must be 1-247");
  uint16_t reg = modbusAddrReg(type, bus);
  if (!modbusWriteReg(from, reg, to)) return sendErr("no reply. Only one sensor on the bus? Correct type and baud?");
  logEvent("Modbus sensor " + String(from) + " -> address " + String(to));
  sendOk();
}

static void handleChannel() {
  JsonDocument d;
  if (!parseBody(d)) return;
  int i = d["i"] | -1;
  String m = d["mode"] | "";
  if (i < 0 || i >= NUM_CHANNELS) return sendErr("bad channel");
  for (uint8_t k = 0; k <= MODE_AUTO; k++)
    if (m == MODE_NAMES[k]) {
      S.modes[i] = k;
      ch[i].lockout = false;  // any mode change counts as a resume
      if (k == MODE_AUTO) ch[i].lastChange = millis() - S.minCycleS * 1000UL;  // react immediately
      saveSettings();
      logEvent(S.names[i] + " mode -> " + m);
      control();
      return sendOk();
    }
  sendErr("bad mode");
}

static void handleResume() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    ch[i].lockout = false;
    ch[i].lastChange = millis() - S.minCycleS * 1000UL;
  }
  logEvent("Safety stop reset by user");
  control();
  sendOk();
}

static void handleScan() {
  int n = WiFi.scanComplete();
  if (server.hasArg("start")) {
    if (n != WIFI_SCAN_RUNNING) {
      WiFi.scanDelete();
      WiFi.scanNetworks(true);
    }
    server.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  if (n == WIFI_SCAN_RUNNING) { server.send(200, "application/json", "{\"scanning\":true}"); return; }
  if (n >= 0) {
    JsonDocument d;
    JsonArray a = d.to<JsonArray>();
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.isEmpty()) continue;
      bool dup = false;
      for (JsonObject o : a)
        if (o["ssid"] == ssid) { dup = true; if (WiFi.RSSI(i) > (int)o["rssi"]) o["rssi"] = WiFi.RSSI(i); }
      if (dup) continue;
      JsonObject o = a.add<JsonObject>();
      o["ssid"] = ssid;
      o["rssi"] = WiFi.RSSI(i);
      o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }
    scanCache = "";
    serializeJson(d, scanCache);
    WiFi.scanDelete();
  }
  server.send(200, "application/json", "{\"scanning\":false,\"networks\":" + scanCache + "}");
}

static void handleWifi() {
  JsonDocument d;
  if (!parseBody(d)) return;
  String ssid = d["ssid"] | "";
  if (ssid.isEmpty() || ssid.length() > 32) return sendErr("bad ssid");
  S.staSsid = ssid;
  S.staPass = d["pass"] | "";
  saveSettings();
  logEvent("Connecting to Wi-Fi " + ssid);
  sendOk();
  delay(100);
  WiFi.disconnect(false);
  startSTA();
}

static void handleWifiForget() {
  S.staSsid = "";
  S.staPass = "";
  saveSettings();
  WiFi.disconnect(false);
  logEvent("Home Wi-Fi forgotten");
  sendOk();
}

static void handleTestAlert() {
  if (S.webhook.isEmpty()) return sendErr("set a notification URL first");
  notify("Test notification from HumidESP" + (isnan(ctlRh) ? String("") : " (RH " + String(ctlRh, 0) + "%)"));
  if (S.buzzer) beepsLeft = 2;
  sendOk();
}

static void handleNotFound() {
  // Captive portal: send anything unknown on the AP to the UI.
  if (server.client().localIP() == AP_IP) {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "not found");
  }
}

// API route that needs a login once a web password is set.
// GitHub release updates: checked every 6 h on home Wi-Fi, installed from loop() so the HTTP reply goes out first.
static bool otaInstallReq = false;
static uint32_t lastOtaMs = 0;

static void otaRun() {
  for (int i = 0; i < NUM_CHANNELS; i++) setChannel(i, false, "firmware update");
  logEvent("Installing firmware " + ota.latest);
  if (otaInstall()) { logEvent("Firmware update OK, restarting"); restartAt = millis() + 500; }
  else logEvent("Firmware update failed: " + ota.error);
}

static void serviceOta() {
  if (otaInstallReq) { otaInstallReq = false; return otaRun(); }
  if (WiFi.status() != WL_CONNECTED || millis() < 120000 || (lastOtaMs && millis() - lastOtaMs < 6 * 3600000UL)) return;
  lastOtaMs = millis();
  // dev builds (flashed over USB) never replace themselves; install a release by hand once
  if (otaCheck() && ota.available && S.autoUpdate && otaIsRelease(FW_VERSION)) otaRun();
}

static void onApi(const char *uri, HTTPMethod m, WebServer::THandlerFunction fn) {
  server.on(uri, m, [fn] { if (authorized()) fn(); else sendLoginRequired(); });
}

static void setupServer() {
  static const char *hdrs[] = {"Cookie"};
  server.collectHeaders(hdrs, 1);
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/auth", HTTP_GET, handleAuth);
  server.on("/api/login", HTTP_POST, handleLogin);
  // Home-screen app (PWA) files, public so the phone can fetch them before login.
  server.on("/manifest.json", HTTP_GET, [] { server.send_P(200, "application/manifest+json", MANIFEST); });
  auto png = [](const uint8_t *p, size_t n) {
    return [p, n] { server.sendHeader("Cache-Control", "max-age=604800"); server.send_P(200, "image/png", (PGM_P)p, n); };
  };
  server.on("/icon-180.png", HTTP_GET, png(ICON_180, sizeof(ICON_180)));
  server.on("/icon-512.png", HTTP_GET, png(ICON_512, sizeof(ICON_512)));
  onApi("/api/ota/check", HTTP_POST, [] { otaCheck(); handleState(); });
  onApi("/api/ota/install", HTTP_POST, [] {
    if (!ota.available) return sendErr("No update available");
    sendOk();
    otaInstallReq = true;
  });
  onApi("/api/logout", HTTP_POST, handleLogout);
  onApi("/api/password", HTTP_POST, handlePassword);
  onApi("/api/state", HTTP_GET, handleState);
  onApi("/api/history", HTTP_GET, handleHistory);
  onApi("/api/log", HTTP_GET, handleLog);
  onApi("/api/settings", HTTP_POST, handleSettings);
  onApi("/api/sensors", HTTP_POST, handleSensors);
  onApi("/api/bus", HTTP_POST, handleBus);
  onApi("/api/modbus/scan", HTTP_POST, handleModbusScan);
  onApi("/api/modbus/setaddr", HTTP_POST, handleModbusSetAddr);
  onApi("/api/channel", HTTP_POST, handleChannel);
  onApi("/api/resume", HTTP_POST, handleResume);
  onApi("/api/scan", HTTP_GET, handleScan);
  onApi("/api/wifi", HTTP_POST, handleWifi);
  onApi("/api/wifi/forget", HTTP_POST, handleWifiForget);
  onApi("/api/test-alert", HTTP_POST, handleTestAlert);
  onApi("/api/reboot", HTTP_POST, [] { sendOk(); restartAt = millis() + 500; });
  onApi("/api/factory-reset", HTTP_POST, [] {
    clearAuth();  // sessions live in "humid" too, this also drops them from RAM
    prefs.begin("humid", false);
    prefs.clear();
    prefs.end();
    sendOk();
    restartAt = millis() + 500;
  });
  // Firmware update: upload .pio/build/esp32dev/firmware.bin from the System section.
  // Auth is checked when the upload starts, so nothing is written without a login.
  server.on("/api/update", HTTP_POST, [] {
    bool denied = uploadDenied || !authorized();
    uploadDenied = false;
    if (denied) return sendLoginRequired();
    bool ok = !Update.hasError();
    server.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"update failed\"}");
    if (ok) restartAt = millis() + 1000;
  }, [] {
    HTTPUpload &up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
      uploadDenied = !authorized();
      if (uploadDenied) return logEvent("Firmware upload refused: not logged in");
      for (int i = 0; i < NUM_CHANNELS; i++) setChannel(i, false, "firmware update");
      logEvent("Firmware upload: " + up.filename);
      Update.begin(UPDATE_SIZE_UNKNOWN);
    } else if (uploadDenied) {
      return;  // nothing was begun, discard the rest of the body
    } else if (up.status == UPLOAD_FILE_WRITE) {
      Update.write(up.buf, up.currentSize);
    } else if (up.status == UPLOAD_FILE_END) {
      if (Update.end(true)) logEvent("Firmware update OK, " + String(up.totalSize) + " bytes");
    } else if (up.status == UPLOAD_FILE_ABORTED) {
      Update.abort();
    }
  });
  server.onNotFound(handleNotFound);
  server.begin();
}

// ---------------------------------------------------------------------------
void setup() {
  // Drive relays to OFF before anything else so they don't click at boot.
  for (int i = 0; i < NUM_CHANNELS; i++) {
    writeRelay(i, false);
    pinMode(RELAY_PINS[i], OUTPUT);
    writeRelay(i, false);
  }
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(BUTTON_PINS[i], OUTPUT);
    digitalWrite(BUTTON_PINS[i], LOW);
  }
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_BUZZER, !BUZZER_ACTIVE_HIGH);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, !BUZZER_ACTIVE_HIGH);
  pinMode(PIN_WATER, INPUT_PULLUP);
  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);

  Serial.begin(115200);
  delay(200);
  Serial.println("\nHumidESP starting");

  hist = (HistPoint *)calloc(HISTORY_LEN, sizeof(HistPoint));
  if (!hist) Serial.println("history alloc failed");

  loadSettings();
  sensorsBegin(bus);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(MDNS_HOST);
  WiFi.setAutoReconnect(true);
  startAP();
  startSTA();
  dns.start(53, "*", AP_IP);
  if (MDNS.begin(MDNS_HOST)) MDNS.addService("http", "tcp", 80);
  setupServer();

  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) {
    logEvent("Wi-Fi connected, IP " + WiFi.localIP().toString());
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

  lastHistMs = millis();
  logEvent("Boot. Hotspot " + String(AP_SSID) + " at 192.168.4.1, " + String(sensors.size()) + " sensor(s)");
}

void loop() {
  dns.processNextRequest();
  server.handleClient();

  if (millis() - lastPollMs >= SENSOR_POLL_MS) {
    lastPollMs = millis();
    pollSensor();
  }
  if (millis() - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = millis();
    control();
    recordHistory();
  }

  serviceButtons();
  checkBootButton();
  updateIndicators();
  maintainWifi();
  processNotifications();
  serviceOta();

  if (apRestartAt && millis() > apRestartAt) { apRestartAt = 0; startAP(); logEvent("Hotspot password changed"); }
  if (restartAt && millis() > restartAt) ESP.restart();
  delay(2);
}
