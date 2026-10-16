#pragma once
#include <Arduino.h>

#ifndef FW_VERSION
#define FW_VERSION "dev"
#endif
#define OTA_REPO "trongio/HumidESP"

struct OtaState {
  String latest;       // newest release tag on GitHub
  String url;          // its firmware.bin download URL
  String error;
  uint32_t checkedS = 0; // uptime of last check, 0 = never
  bool available = false; // newer than the running firmware
};
extern OtaState ota;

bool otaIsRelease(const String &v); // "v1.2.3" style tag (not a dev build)
bool otaCheck();                    // blocking, a second or two
bool otaInstall();                  // blocking; restart afterwards on success
