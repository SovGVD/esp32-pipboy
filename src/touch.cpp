#include <SPI.h>
#include <Touch.h>
#include "Pipboy.h"

namespace {
const touch::Calibration calibration = {
  hardware::touchRawXMin, hardware::touchRawXMax,
  hardware::touchRawYMin, hardware::touchRawYMax,
  hardware::touchSwapXY, hardware::touchInvertX, hardware::touchInvertY,
  hardware::touchPressureMin
};
const SPISettings touchSpi(2000000, MSBFIRST, SPI_MODE0);
touch::TapDetector tapDetector;
bool isTouchReady = false;
bool wasTouchDown = false;

// 12-bit differential conversion with PD1:PD0=00, so the chip powers down and
// re-enables PENIRQ after each sample. The reply is a null bit, 12 data bits
// and three trailing bits.
int touchReadAdc(uint8_t command)
{
  SPI.transfer(command);
  const uint8_t high = SPI.transfer(0);
  const uint8_t low = SPI.transfer(0);
  return ((high << 8) | low) >> 3 & 0xfff;
}

bool touchSample(int& rawX, int& rawY, int& pressure)
{
  if (digitalRead(hardware::touchIrq)) {
    return false;
  }

  // The transaction lock serializes this with the SD reads of the music task.
  int xs[5], ys[5];
  SPI.beginTransaction(touchSpi);
  digitalWrite(hardware::touchCs, LOW);
  const int z1 = touchReadAdc(0xb0);
  const int z2 = touchReadAdc(0xc0);
  pressure = z1 + 4095 - z2;
  const bool isPressed = pressure >= calibration.pressureMin;
  for (uint8_t idx = 0; isPressed && idx < 5; idx++) {
    // Discard the first conversion after switching the panel drive axis.
    touchReadAdc(0xd0);
    xs[idx] = touchReadAdc(0xd0);
    touchReadAdc(0x90);
    ys[idx] = touchReadAdc(0x90);
  }
  digitalWrite(hardware::touchCs, HIGH);
  SPI.endTransaction();

  if (!isPressed) {
    return false;
  }

  rawX = touch::median5(xs);
  rawY = touch::median5(ys);
  return true;
}
}

void touchSetup()
{
  // Keep both devices on the shared bus deselected, then start the bus here so
  // the music task's later SPI.begin() is a no-op instead of a race.
  pinMode(hardware::touchCs, OUTPUT);
  digitalWrite(hardware::touchCs, HIGH);
  pinMode(hardware::sdCs, OUTPUT);
  digitalWrite(hardware::sdCs, HIGH);
  pinMode(hardware::touchIrq, INPUT_PULLUP);
  SPI.begin(hardware::sdSck, hardware::sdMiso, hardware::sdMosi, hardware::sdCs);

  SPI.beginTransaction(touchSpi);
  digitalWrite(hardware::touchCs, LOW);
  touchReadAdc(0x90); // power down so PENIRQ is enabled
  digitalWrite(hardware::touchCs, HIGH);
  SPI.endTransaction();

  isTouchReady = true;
  Serial.printf("Resistive touch ready: CS=%d IRQ=%d\n", hardware::touchCs, hardware::touchIrq);
}

void touchUpdate()
{
  isTouchTapped = false;
  if (!isTouchReady) {
    return;
  }

  int rawX = 0, rawY = 0, pressure = 0, x = -1, y = -1;
  const bool isDown = touchSample(rawX, rawY, pressure) &&
    touch::map(calibration, rawX, rawY, w, h, x, y);
  if (isDown && !wasTouchDown) {
    Serial.printf("Touch raw=%d,%d pressure=%d screen=%d,%d\n", rawX, rawY, pressure, x, y);
  }
  wasTouchDown = isDown;

  if (tapDetector.update(isDown, x, y)) {
    isTouchTapped = true;
    touchX = tapDetector.x();
    touchY = tapDetector.y();
  }
}
