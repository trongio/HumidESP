// Firmware updates from GitHub releases (asset "firmware.bin"), verified TLS.
#include "ota.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_timer.h>
#include "github_ca.h"

OtaState ota;

// Parses "v1.2.3" into a comparable number; 0 when not a release tag.
static uint32_t semver(const String &v) {
  int a, b, c;
  char tail;
  if (sscanf(v.c_str(), "v%d.%d.%d%c", &a, &b, &c, &tail) != 3) return 0;
  return (a << 20) | (b << 10) | c;
}

bool otaIsRelease(const String &v) { return semver(v) != 0; }

bool otaCheck() {
  WiFiClientSecure tls;
  tls.setCACert(GITHUB_CA);
  HTTPClient http;
  http.useHTTP10(true);  // plain stream, no chunked decoding needed
  http.setTimeout(8000);
  http.begin(tls, "https://api.github.com/repos/" OTA_REPO "/releases/latest");
  http.setUserAgent("HumidESP/" FW_VERSION);
  int code = http.GET();
  ota.checkedS = esp_timer_get_time() / 1000000ULL;
  if (code != 200) {
    ota.error = code == 404 ? "no releases yet" : "GitHub check failed (" + String(code) + ")";
    http.end();
    return false;
  }
  JsonDocument filter, d;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  DeserializationError err = deserializeJson(d, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { ota.error = "bad reply from GitHub"; return false; }
  ota.latest = d["tag_name"] | "";
  ota.url = "";
  for (JsonObject a : d["assets"].as<JsonArray>())
    if (a["name"] == "firmware.bin") ota.url = a["browser_download_url"].as<String>();
  // A dev build can always "update" to a release; releases only move forward.
  uint32_t cur = semver(FW_VERSION), next = semver(ota.latest);
  ota.available = ota.url.length() && next && (!cur || next > cur);
  ota.error = ota.url.length() ? "" : "release has no firmware.bin";
  return true;
}

bool otaInstall() {
  if (ota.url.isEmpty()) { ota.error = "check for updates first"; return false; }
  WiFiClientSecure tls;
  tls.setCACert(GITHUB_CA);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // GitHub redirects to its asset CDN
  http.begin(tls, ota.url);
  int code = http.GET();
  int len = http.getSize();
  if (code != 200 || len <= 0) {
    ota.error = "download failed (" + String(code) + ")";
    http.end();
    return false;
  }
  bool ok = Update.begin(len) && Update.writeStream(http.getStream()) == (size_t)len && Update.end(true);
  if (!ok) {
    ota.error = String("flash failed: ") + Update.errorString();
    Update.abort();
  }
  http.end();
  return ok;
}
