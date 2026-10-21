#pragma once

#include <cstdint>

#define PIPBOY_SETTINGS_VERSION 0x0001
#define PIPBOY_TOUCH_CALIBRATION_MAGIC 0x7C41

// Appended after the name so older EEPROM contents keep their name; the magic
// number tells whether a calibration was ever saved.
struct pipboyTouchCalibration {
  uint16_t magic;
  int16_t xMin, xMax, yMin, yMax;
  uint8_t swapXY, invertX, invertY;
  uint8_t reserved;
  int16_t pressureMin;
};

struct pipboySettings {
  uint16_t version;
  char pipboyName[16];
  pipboyTouchCalibration touch;
};
