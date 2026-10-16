#pragma once
#include <Arduino.h>
#include "config.h"

enum SensorType : uint8_t {
  ST_DHT11,
  ST_DHT22,
  ST_SHT3X,      // I2C 0x44 / 0x45
  ST_SHT4X,      // I2C 0x44 / 0x45, with periodic anti-condensation heater
  ST_MB_XYMD02,  // RS485 Modbus, XY-MD02 / XY-MD03 register map (FC4, reg 1 temp, reg 2 RH)
  ST_MB_RENKE,   // RS485 Modbus, Renke RS-WS-N01 style (FC3, reg 0 RH, reg 1 temp, default 4800 baud)
  ST_MB_CUSTOM,  // RS485 Modbus, register map from bus settings
  ST_COUNT
};
extern const char *SENSOR_TYPE_KEYS[ST_COUNT];
int sensorTypeFromKey(const String &k);
bool sensorIsModbus(uint8_t type);

struct SensorCfg {
  uint8_t type = ST_DHT11;
  uint8_t addr = 0;     // Modbus address, or I2C address (+0x80 = second I2C bus)
  String name;
  float x = 0.5, y = 0.5; // position in room, 0..1 of width / length
  float z = 1.5;          // height above floor in metres
  bool control = true;    // included in the control value
  float hOff = 0, tOff = 0; // calibration offsets
};

// Per-sensor runtime data used by the drivers.
struct SensorRuntime {
  uint32_t lastHeatMs = 0;
  uint32_t skipUntilMs = 0;
  float lastRh = 0;
};

struct ModbusBus {
  uint32_t baud = 9600;
  uint8_t fc = 3;        // custom profile: function code 3 or 4
  uint16_t humReg = 0;   // custom profile: register with RH x10
  uint16_t tempReg = 1;  // custom profile: register with temp x10 (signed)
  uint16_t addrReg = 0x07D0; // custom profile: holding register for slave address
};

void sensorsBegin(const ModbusBus &bus);
// Returns true and fills rh/t when a fresh reading was taken.
bool sensorRead(const SensorCfg &cfg, SensorRuntime &rt, const ModbusBus &bus, float &rh, float &t);
bool modbusReadRegs(uint8_t addr, uint8_t fc, uint16_t reg, uint16_t count, uint16_t *out);
bool modbusWriteReg(uint8_t addr, uint16_t reg, uint16_t value);
uint16_t modbusAddrReg(uint8_t type, const ModbusBus &bus);
