#pragma once

#include <cstdint>

#define PIPBOY_SETTINGS_VERSION 0x0001

struct pipboySettings {
  uint16_t version;
  char pipboyName[16];
};
