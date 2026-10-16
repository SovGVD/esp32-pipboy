#include "Pipboy.h"
#include "Hardware.h"

// Display uses SPI3 (HSPI); the SD card uses the default SPI2 (FSPI).
static Arduino_DataBus* bus = new Arduino_ESP32QSPI(45, 47, 21, 48, 40, 39);
static Arduino_GFX* panel = new Arduino_NV3041A(bus, GFX_NOT_DEFINED, 0, true);
Arduino_GFX* gfx = new Arduino_Canvas(480, 272, panel);
