#include <SPI.h>
#include <Touch.h>
#include "Pipboy.h"

namespace {
const touch::Calibration defaultCalibration = {
  hardware::touchRawXMin, hardware::touchRawXMax,
  hardware::touchRawYMin, hardware::touchRawYMax,
  hardware::touchSwapXY, hardware::touchInvertX, hardware::touchInvertY,
  hardware::touchPressureMin
};
touch::Calibration calibration = defaultCalibration;
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


touch::Calibration touchSavedCalibration()
{
  const pipboyTouchCalibration& saved = settings.touch;
  const touch::Calibration result = {
    saved.xMin, saved.xMax, saved.yMin, saved.yMax,
    saved.swapXY != 0, saved.invertX != 0, saved.invertY != 0,
    saved.pressureMin
  };
  return result;
}

void touchSaveCalibration(const touch::Calibration& found)
{
  pipboyTouchCalibration& saved = settings.touch;
  saved.magic = PIPBOY_TOUCH_CALIBRATION_MAGIC;
  saved.xMin = found.xMin;
  saved.xMax = found.xMax;
  saved.yMin = found.yMin;
  saved.yMax = found.yMax;
  saved.swapXY = found.swapXY;
  saved.invertX = found.invertX;
  saved.invertY = found.invertY;
  saved.reserved = 0;
  saved.pressureMin = found.pressureMin;
  settingsCommit();
  calibration = found;
  Serial.printf("Touch calibration saved: x=%d..%d y=%d..%d swap=%d invert=%d,%d\n",
    found.xMin, found.xMax, found.yMin, found.yMax, found.swapXY, found.invertX, found.invertY);
}

void touchDrawCross(int16_t x, int16_t y)
{
  gfx->drawLine(x - 12, y, x + 12, y, COLOR_MAIN);
  gfx->drawLine(x, y - 12, x, y + 12, COLOR_MAIN);
  gfx->drawRect(x - 3, y - 3, 7, 7, COLOR_MAIN);
}
}

bool touchIsHeld()
{
  // Three samples in a row, so one noisy reading does not count.
  for (uint8_t idx = 0; isTouchReady && idx < 3; idx++) {
    int rawX, rawY, pressure;
    if (!touchSample(rawX, rawY, pressure)) {
      return false;
    }
    delay(20);
  }
  return isTouchReady;
}

bool touchHasSavedCalibration()
{
  return settings.touch.magic == PIPBOY_TOUCH_CALIBRATION_MAGIC && touch::valid(touchSavedCalibration());
}

/**
 * Blocking calibration screen: four corner targets, then a center check.
 * The dial click skips (first boot, keeping the defaults) or cancels.
 */
void touchCalibrate(bool isFirstBoot)
{
  const uint8_t verifyStep = touch::calibrationTargets;
  touch::PressCapture capture;
  touch::Sample raw[touch::calibrationTargets] = {};
  touch::Calibration found = calibration;
  uint8_t step = 0;
  const char* status = "";

  const bool wasSubMenuEncoderControlList = isSubMenuEncoderControlList;
  isSubMenuEncoderControlList = true; // so a click is reported, not used to enter a list
  resetSubMenuButton();

  while (isTouchReady) {
    buttonsUpdate();
    bootButtonUpdate();
    currentTime = micros();
    if (currentTime - previousTime < LOOP_TIME) {
      delay(1);
      continue;
    }
    previousTime = currentTime;

    if (isSubMenuButtonClicked || isSubMenuButtonLongPressed || bootButtonClicked()) {
      if (isFirstBoot) {
        // Do not ask again on every boot; CALIBRATE TOUCH in setup mode remains.
        touchSaveCalibration(defaultCalibration);
      }
      break;
    }

    int rawX = 0, rawY = 0, pressure = 0;
    const bool isDown = touchSample(rawX, rawY, pressure);
    if (capture.update(isDown, rawX, rawY)) {
      if (step < verifyStep) {
        raw[step++] = capture.result();
        status = "";
        if (step == verifyStep && !touch::calibrate(raw, w, h, hardware::touchPressureMin, found)) {
          step = 0;
          status = "Taps too close. Try again.";
        }
      } else {
        int x = -1, y = -1;
        const touch::Sample check = capture.result();
        if (touch::map(found, check.x, check.y, w, h, x, y) && abs(x - w / 2) <= 20 && abs(y - h / 2) <= 20) {
          touchSaveCalibration(found);
          break;
        }
        Serial.printf("Touch check missed: screen=%d,%d\n", x, y);
        step = 0;
        status = "Check missed. Try again.";
      }
    }

    gfx->fillScreen(COLOR_SCREEN);
    gfx->setTextColor(COLOR_MAIN);
    gfx->setCursor(105, 80);
    gfx->print(step < verifyStep ? "Tap the center of the cross" : "Check: tap the center cross");
    gfx->setCursor(105, 104);
    gfx->printf("TOUCH CALIBRATION %d/%d", step + 1, verifyStep + 1);
    gfx->setCursor(105, 128);
    gfx->print(isFirstBoot ? "Click dial or BOOT to skip" : "Click dial or BOOT to cancel");
    gfx->setCursor(105, 200);
    gfx->print(status);

    int16_t targetX = w / 2, targetY = h / 2;
    if (step < verifyStep) {
      int x, y;
      touch::calibrationTarget(step, w, h, x, y);
      targetX = x;
      targetY = y;
    }
    touchDrawCross(targetX, targetY);

    #ifdef CANVAS
      gfx->flush();
    #endif
    resetSubMenuButton();
  }

  isSubMenuEncoderControlList = wasSubMenuEncoderControlList;
  resetSubMenuButton();
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

  if (touchHasSavedCalibration()) {
    calibration = touchSavedCalibration();
  }

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
  isTouchDown = isDown;
  if (isDown) {
    touchDownX = x;
    touchDownY = y;
  }

  if (tapDetector.update(isDown, x, y)) {
    isTouchTapped = true;
    touchX = tapDetector.x();
    touchY = tapDetector.y();
  }
}
