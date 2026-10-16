#pragma once

// ---------------------------------------------------------------------------
// Hardware pins (ESP32-WROOM-32 DevKit, 30 pin). See docs/wiring-overview.svg.
// ---------------------------------------------------------------------------
#define PIN_DHT          4    // DHT11/DHT22 data (desk testing only, not for cold rooms)

#define PIN_SDA          21   // I2C for SHT3x / SHT4x breakouts (short cables only)
#define PIN_SCL          22
#define PIN_SDA2         19   // second I2C bus, so two probes with the same address can be used
#define PIN_SCL2         23

#define PIN_RS485_RX     16   // RS485 module RO / RXD
#define PIN_RS485_TX     17   // RS485 module DI / TXD
#define PIN_RS485_DE     18   // DE+RE tied together (unused by auto-direction modules)

#define NUM_CHANNELS     2
static const int RELAY_PINS[NUM_CHANNELS] = {26, 27}; // humidifier 1, 2

// Most red relay modules have an H/L trigger jumper. Put it on H and leave this
// true. If your module is low-level trigger only, set this to false.
#define RELAY_ACTIVE_HIGH true

// Optional: PC817 optocoupler across the atomizer board's push button, for driver
// boards that power up idle and need a button press to start misting.
// Set "button presses after power on" per humidifier in the web UI (0 = not used).
static const int BUTTON_PINS[NUM_CHANNELS] = {32, 13};
#define BUTTON_PRESS_MS  200
#define BUTTON_GAP_MS    400
#define BUTTON_DELAY_MS  1000  // wait after power on before the first press

#define PIN_WATER        33   // optional float switch to GND (enable in web UI)
#define PIN_BUZZER       25   // optional active buzzer module (enable in web UI)
#define BUZZER_ACTIVE_HIGH true // false for low-trigger modules (e.g. dac.ge AR0167)
#define PIN_LED          2    // onboard blue LED
#define PIN_BOOT_BUTTON  0    // onboard BOOT button: hold 10 s while running to clear the web password

// ---------------------------------------------------------------------------
// Wi-Fi access point (always on, so you can reach the UI even without router)
// ---------------------------------------------------------------------------
#define AP_SSID          "HumidESP"
#define AP_DEFAULT_PASS  "humidesp123" // >= 8 chars, changeable in the web UI
#define MDNS_HOST        "humidesp"    // http://humidesp.local on your home LAN

// ---------------------------------------------------------------------------
// Sensors and timing
// ---------------------------------------------------------------------------
#define MAX_SENSORS         8
#define SENSOR_POLL_MS      500    // one sensor read per tick, round robin
#define CONTROL_INTERVAL_MS 2500
#define SENSOR_STALE_MS     60000  // reading older than this = sensor offline
#define HISTORY_INTERVAL_S  60     // one chart point per minute
#define HISTORY_LEN         1440   // 24 h of points
#define NO_RISE_CHECK_MIN   20     // humidifier on this long without +1% RH -> alert
