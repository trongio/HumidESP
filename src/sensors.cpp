#include "sensors.h"
#include <DHT.h>
#include <Wire.h>

const char *SENSOR_TYPE_KEYS[ST_COUNT] = {"dht11", "dht22", "sht3x", "sht4x", "xymd02", "renke", "modbus"};

static DHT dht11(PIN_DHT, DHT11);
static DHT dht22(PIN_DHT, DHT22);

int sensorTypeFromKey(const String &k) {
  for (int i = 0; i < ST_COUNT; i++)
    if (k == SENSOR_TYPE_KEYS[i]) return i;
  return -1;
}

bool sensorIsModbus(uint8_t type) { return type == ST_MB_XYMD02 || type == ST_MB_RENKE || type == ST_MB_CUSTOM; }

void sensorsBegin(const ModbusBus &bus) {
  dht11.begin();
  dht22.begin();
  Wire.begin(PIN_SDA, PIN_SCL, 50000);  // slow clock tolerates longer wires
  Wire.setTimeOut(50);
  pinMode(PIN_RS485_DE, OUTPUT);
  digitalWrite(PIN_RS485_DE, LOW);
  Serial2.end();
  Serial2.begin(bus.baud, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
}

// ---------------------------------------------------------------------------
// Sensirion SHT3x / SHT4x
// ---------------------------------------------------------------------------
static uint8_t crc8(const uint8_t *d, int n) {
  uint8_t c = 0xFF;
  for (int i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++) c = (c & 0x80) ? (c << 1) ^ 0x31 : c << 1;
  }
  return c;
}

static bool i2cCommand(uint8_t addr, const uint8_t *cmd, int len) {
  Wire.beginTransmission(addr);
  Wire.write(cmd, len);
  return Wire.endTransmission() == 0;
}

static bool i2cRead6(uint8_t addr, uint16_t &a, uint16_t &b) {
  uint8_t d[6];
  if (Wire.requestFrom(addr, (uint8_t)6) != 6) return false;
  for (auto &x : d) x = Wire.read();
  if (crc8(d, 2) != d[2] || crc8(d + 3, 2) != d[5]) return false;
  a = (d[0] << 8) | d[1];
  b = (d[3] << 8) | d[4];
  return true;
}

static bool readSht3x(uint8_t addr, float &rh, float &t) {
  const uint8_t cmd[] = {0x24, 0x00};  // single shot, high repeatability, no stretching
  if (!i2cCommand(addr, cmd, 2)) return false;
  delay(16);
  uint16_t rt, rr;
  if (!i2cRead6(addr, rt, rr)) return false;
  t = -45 + 175.0f * rt / 65535.0f;
  rh = 100.0f * rr / 65535.0f;
  return true;
}

static bool readSht4x(uint8_t addr, SensorRuntime &rt, float &rh, float &t) {
  uint32_t now = millis();
  if (now < rt.skipUntilMs) return false;  // still cooling after a heater pulse
  // Near saturation the sensor can collect condensation and read high.
  // A short heater pulse every 10 minutes keeps it honest.
  if (rt.lastRh > 95 && now - rt.lastHeatMs > 600000) {
    const uint8_t heat[] = {0x39};  // 200 mW for 1 s, then measure
    rt.lastHeatMs = now;
    if (i2cCommand(addr, heat, 1)) delay(1100);
    uint16_t a, b;
    i2cRead6(addr, a, b);  // discard the hot reading
    rt.skipUntilMs = millis() + 30000;
    return false;
  }
  const uint8_t cmd[] = {0xFD};  // high precision measurement
  if (!i2cCommand(addr, cmd, 1)) return false;
  delay(10);
  uint16_t r1, r2;
  if (!i2cRead6(addr, r1, r2)) return false;
  t = -45 + 175.0f * r1 / 65535.0f;
  rh = constrain(-6 + 125.0f * r2 / 65535.0f, 0.0f, 100.0f);
  return true;
}

// ---------------------------------------------------------------------------
// Modbus RTU
// ---------------------------------------------------------------------------
static uint16_t crc16(const uint8_t *d, int n) {
  uint16_t c = 0xFFFF;
  for (int i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++) c = (c & 1) ? (c >> 1) ^ 0xA001 : c >> 1;
  }
  return c;
}

static bool modbusTransact(uint8_t *req, int reqLen, uint8_t *resp, int respLen) {
  uint16_t c = crc16(req, reqLen - 2);
  req[reqLen - 2] = c & 0xFF;
  req[reqLen - 1] = c >> 8;
  while (Serial2.available()) Serial2.read();
  delay(4);  // inter-frame gap
  digitalWrite(PIN_RS485_DE, HIGH);
  Serial2.write(req, reqLen);
  Serial2.flush();
  digitalWrite(PIN_RS485_DE, LOW);

  int got = 0;
  uint32_t start = millis();
  while (got < respLen && millis() - start < 200) {
    if (Serial2.available()) {
      resp[got++] = Serial2.read();
      // Exception reply is 5 bytes long.
      if (got == 5 && (resp[1] & 0x80)) return false;
    } else {
      delay(1);
    }
  }
  if (got < respLen || resp[0] != req[0]) return false;
  uint16_t rc = crc16(resp, respLen - 2);
  return resp[respLen - 2] == (rc & 0xFF) && resp[respLen - 1] == (rc >> 8);
}

bool modbusReadRegs(uint8_t addr, uint8_t fc, uint16_t reg, uint16_t count, uint16_t *out) {
  if (count == 0 || count > 16) return false;
  uint8_t req[8] = {addr, fc, (uint8_t)(reg >> 8), (uint8_t)reg, 0, (uint8_t)count, 0, 0};
  uint8_t resp[5 + 32];
  int len = 5 + count * 2;
  if (!modbusTransact(req, 8, resp, len)) return false;
  if (resp[1] != fc || resp[2] != count * 2) return false;
  for (int i = 0; i < count; i++) out[i] = (resp[3 + i * 2] << 8) | resp[4 + i * 2];
  return true;
}

bool modbusWriteReg(uint8_t addr, uint16_t reg, uint16_t value) {
  uint8_t req[8] = {addr, 0x06, (uint8_t)(reg >> 8), (uint8_t)reg, (uint8_t)(value >> 8), (uint8_t)value, 0, 0};
  uint8_t resp[8];
  return modbusTransact(req, 8, resp, 8) && resp[1] == 0x06;
}

uint16_t modbusAddrReg(uint8_t type, const ModbusBus &bus) {
  switch (type) {
    case ST_MB_XYMD02: return 0x0101;
    case ST_MB_RENKE: return 0x07D0;
    default: return bus.addrReg;
  }
}

static bool readModbus(const SensorCfg &cfg, const ModbusBus &bus, float &rh, float &t) {
  uint8_t fc;
  uint16_t hr, tr;
  switch (cfg.type) {
    case ST_MB_XYMD02: fc = 4; tr = 1; hr = 2; break;
    case ST_MB_RENKE: fc = 3; hr = 0; tr = 1; break;
    default: fc = bus.fc; hr = bus.humReg; tr = bus.tempReg; break;
  }
  uint16_t lo = min(hr, tr), span = max(hr, tr) - lo + 1;
  uint16_t v[16];
  if (span <= 16) {
    if (!modbusReadRegs(cfg.addr, fc, lo, span, v)) return false;
    rh = v[hr - lo] / 10.0f;
    t = (int16_t)v[tr - lo] / 10.0f;
  } else {
    uint16_t a, b;
    if (!modbusReadRegs(cfg.addr, fc, hr, 1, &a) || !modbusReadRegs(cfg.addr, fc, tr, 1, &b)) return false;
    rh = a / 10.0f;
    t = (int16_t)b / 10.0f;
  }
  return true;
}

// ---------------------------------------------------------------------------
bool sensorRead(const SensorCfg &cfg, SensorRuntime &rt, const ModbusBus &bus, float &rh, float &t) {
  bool ok = false;
  switch (cfg.type) {
    case ST_DHT11:
    case ST_DHT22: {
      DHT &d = cfg.type == ST_DHT11 ? dht11 : dht22;
      rh = d.readHumidity(true);
      t = d.readTemperature();
      ok = !isnan(rh) && !isnan(t);
      break;
    }
    case ST_SHT3X: ok = readSht3x(cfg.addr, rh, t); break;
    case ST_SHT4X: ok = readSht4x(cfg.addr, rt, rh, t); break;
    default: ok = readModbus(cfg, bus, rh, t); break;
  }
  if (!ok || rh < 0 || rh > 100 || t < -40 || t > 85) return false;
  rt.lastRh = rh;
  return true;
}
