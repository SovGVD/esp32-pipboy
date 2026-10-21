#include "Pipboy.h"
bool didWeJustEnteredToCharEdit = false;
bool didWeJustSubmitOneCharEdit = false;
bool preIsSubMenuEncoderControlList = false;

enum class SetupPage : uint8_t { Menu, Name };
SetupPage setupPage = SetupPage::Menu;

#define SETUP_MENU_ITEMS 3
#define SETUP_MENU_Y 87

// Touch areas of the name editor, matching what setupPipboyName() draws.
#define SETUP_NAME_CELL_W 14
#define SETUP_NAME_TOP 70
#define SETUP_NAME_BOTTOM 115
#define SETUP_SUBMIT_BOTTOM 152
#define SETUP_PALETTE_X 5
#define SETUP_PALETTE_TOP 166
#define SETUP_PALETTE_COLUMNS 20
#define SETUP_PALETTE_CELL_W 23
#define SETUP_PALETTE_CELL_H 30

menuItem setupModeMenu[SETUP_MENU_ITEMS] = {
  {"CHANGE NAME",     11, {0, 0, 0, 0}, setupMenuChangeName},
  {"CALIBRATE TOUCH", 15, {0, 0, 0, 0}, setupMenuCalibrateTouch},
  {"SAVE AND EXIT",   13, {0, 0, 0, 0}, setupMenuExit},
};

void setupMode()
{
  Serial.println("Setup mode");
  isInSetupMode = true;
  isDisplayMessageWindow = false;
  bootButtonClicked(); // drop presses from before setup mode
  bootButtonHeld();
  setupShowMenu(0);

  setupLoop();

  Serial.println("Setup loop done... Restart");
  ESP.restart();
}

void setupLoop() {
  while(isInSetupMode) {
    buttonsUpdate();
    bootButtonUpdate();
    currentTime = micros();
    if (currentTime - previousTime >= LOOP_TIME) {
      encoderUpdate();
      touchUpdate();
      if (isTouchTapped && setupPage == SetupPage::Menu) {
        touchListTap();
      }
      if (bootButtonClicked() && setupPage == SetupPage::Menu) {
        // Without dials or touch: BOOT click moves down the menu, BOOT hold selects.
        listMenuNum = (listMenuNum + 1) % SETUP_MENU_ITEMS;
      }
      if (bootButtonHeld() && setupPage == SetupPage::Menu) {
        isSubMenuButtonClicked = true;
      }
      touchClearRegions();
      previousTime = currentTime;

      didWeJustEnteredToCharEdit = isSubMenuEncoderControlList && !preIsSubMenuEncoderControlList;
      didWeJustSubmitOneCharEdit = !isSubMenuEncoderControlList && preIsSubMenuEncoderControlList;

      gfx->fillScreen(COLOR_SCREEN);
      gfx->setTextColor(COLOR_MAIN);


      if (setupPage == SetupPage::Menu) {
        setupMenuPage();
      } else {
        setupPipboyName();
      }


      gfx->setCursor(10, CHAR_HEIGHT);
      gfx->setTextColor(COLOR_DEBUG);
      gfx->print(currentTick/30);

      gfx->setCursor(100, CHAR_HEIGHT);
      gfx->print("SETUP MODE");

      #ifdef CANVAS
        gfx->flush();
      #endif
      preIsSubMenuEncoderControlList = isSubMenuEncoderControlList;
      currentTick++;
      resetSubMenuButton();
    }
    delay(1);
  }
}

void setupShowMenu(uint8_t selectedItem)
{
  setupPage = SetupPage::Menu;
  // The menu is a list that is always active, so a click selects an item.
  isSubMenuEncoderControlList = true;
  preIsSubMenuEncoderControlList = true;
  listMenuItems = SETUP_MENU_ITEMS;
  listMenuNum = selectedItem;
}

void setupMenuPage()
{
  isSubMenuEncoderControlList = true; // a hold would otherwise leave the list
  listMenuItems = SETUP_MENU_ITEMS;

  drawListMenu(
    setupModeMenu,
    SETUP_MENU_ITEMS,
    listMenuNum,
    COLOR_MAIN,
    COLOR_SCREEN,
    20,
    SETUP_MENU_Y,
    16
  );

  gfx->setTextColor(COLOR_MAIN);
  gfx->setCursor(20, 200);
  gfx->print("NAME: ");
  gfx->print(settings.pipboyName);
  gfx->setCursor(20, 224);
  gfx->print(touchHasSavedCalibration() ? "TOUCH: CALIBRATED" : "TOUCH: DEFAULT");
  gfx->setCursor(20, 260);
  gfx->print("BOOT: NEXT   HOLD BOOT: SELECT");

  if (isSubMenuButtonClicked) {
    setupModeMenu[listMenuNum].app();
  }
}

void setupMenuChangeName()
{
  setupPage = SetupPage::Name;
  subMenuNum = 0;
  listMenuNum = 0;
  // Start on character selection without treating the switch as a submitted edit.
  isSubMenuEncoderControlList = false;
  preIsSubMenuEncoderControlList = false;
}

void setupMenuCalibrateTouch()
{
  touchCalibrate(false);
  setupShowMenu(1);
}

void setupMenuExit()
{
  settingsCommit();
  isInSetupMode = false;
}

/**
 * Ugly as hell
*/
void setupPipboyName()
{
  uint16_t hlSpacing = 4;
  uint16_t hlLength = CHAR_WIDTH;
  uint16_t hlPositionX = 0;
  uint16_t hlPositionY = 0;

  subMenuItems = sizeof(settings.pipboyName) - 1; // keep the NUL terminator

  if (setupPipboyNameSubmit() || setupPipboyNameTouch()) {
    return;
  }

  if (didWeJustEnteredToCharEdit) {
    listMenuItems = strlen(allowedChars);
    listMenuNum = 0;

    // Find correct listMenuNum
    for (uint8_t idx = 0; allowedChars[idx] != '\0'; idx++)
    {
      if (settings.pipboyName[subMenuNum] == allowedChars[idx]) {
        listMenuNum = idx;
      }
    }
  }

  if (didWeJustSubmitOneCharEdit) {
    setupSetNameChar(subMenuNum, allowedChars[listMenuNum]);
  }

  for(uint8_t idx = 0; idx <= subMenuItems; idx++)
  {
    if (idx == subMenuItems) {
      hlLength = CHAR_WIDTH * 8 + hlSpacing/2; // [SUBMIT]

      hlPositionX = hlSpacing;
      hlPositionY = 140;
    } else {
      hlLength = CHAR_WIDTH + hlSpacing;  // regular character

      hlPositionX = idx * (CHAR_WIDTH + hlSpacing) + hlSpacing/2;
      hlPositionY = 100;
    }

    if (idx == subMenuNum) {
      gfx->fillRect(
        hlPositionX - hlSpacing/2, hlPositionY - CHAR_HEIGHT + 3,
        hlLength, CHAR_HEIGHT,
        isSubMenuEncoderControlList ? COLOR_MAIN : COLOR_INACTIVE);
    }

    gfx->setTextColor(COLOR_MAIN);
    gfx->setCursor(hlPositionX, hlPositionY);
    if (idx == subMenuItems) {
      gfx->print("[SUBMIT]");
    } else {
      if (isSubMenuEncoderControlList && idx == subMenuNum) {
        gfx->setTextColor(COLOR_SCREEN);
        gfx->print(allowedChars[listMenuNum]);
      } else {
        gfx->print(settings.pipboyName[idx]);
      }
    }
  }

  setupDrawNamePalette();

  subMenuItems++; // For [SUBMIT] button
}

void setupSetNameChar(uint8_t position, char symbol)
{
  // Earlier empty positions become spaces so the name does not end before this one.
  for (uint8_t idx = 0; idx < position; idx++) {
    if (settings.pipboyName[idx] == '\0') {
      settings.pipboyName[idx] = ' ';
    }
  }
  settings.pipboyName[position] = symbol;
}

void setupDrawNamePalette()
{
  // Every allowed character, to tap instead of turning the dial.
  const char current = isSubMenuEncoderControlList ? allowedChars[listMenuNum] : settings.pipboyName[subMenuNum];
  for (uint8_t idx = 0; allowedChars[idx] != '\0'; idx++) {
    const int16_t x = SETUP_PALETTE_X + (idx % SETUP_PALETTE_COLUMNS) * SETUP_PALETTE_CELL_W;
    const int16_t y = SETUP_PALETTE_TOP + (idx / SETUP_PALETTE_COLUMNS) * SETUP_PALETTE_CELL_H;
    const bool isCurrent = allowedChars[idx] == current;
    if (isCurrent) {
      gfx->fillRect(x, y, SETUP_PALETTE_CELL_W - 2, SETUP_PALETTE_CELL_H - 2, COLOR_MAIN);
    } else {
      gfx->drawRect(x, y, SETUP_PALETTE_CELL_W - 2, SETUP_PALETTE_CELL_H - 2, COLOR_INACTIVE);
    }
    gfx->setTextColor(isCurrent ? COLOR_SCREEN : COLOR_MAIN);
    gfx->setCursor(x + 6, y + 21);
    gfx->print(allowedChars[idx] == ' ' ? '_' : allowedChars[idx]);
  }
}

/**
 * Tap a name position to select it, a palette character to set it (and move
 * to the next position) or [SUBMIT] to save. Returns true when the page closed.
 */
bool setupPipboyNameTouch()
{
  if (!isTouchTapped) {
    return false;
  }

  const uint8_t positions = sizeof(settings.pipboyName) - 1;
  if (touchY >= SETUP_NAME_TOP && touchY < SETUP_NAME_BOTTOM && touchX < positions * SETUP_NAME_CELL_W) {
    subMenuNum = touchX / SETUP_NAME_CELL_W;
  } else if (touchY >= SETUP_NAME_BOTTOM && touchY < SETUP_SUBMIT_BOTTOM && touchX < CHAR_WIDTH * 9) {
    settingsCommit();
    setupShowMenu(0);
    return true;
  } else if (touchY >= SETUP_PALETTE_TOP && touchX >= SETUP_PALETTE_X) {
    const uint16_t column = (touchX - SETUP_PALETTE_X) / SETUP_PALETTE_CELL_W;
    const uint16_t row = (touchY - SETUP_PALETTE_TOP) / SETUP_PALETTE_CELL_H;
    const uint16_t idx = row * SETUP_PALETTE_COLUMNS + column;
    if (column >= SETUP_PALETTE_COLUMNS || idx >= strlen(allowedChars)) {
      return false;
    }
    setupSetNameChar(subMenuNum < positions ? subMenuNum : positions - 1, allowedChars[idx]);
    if (subMenuNum < positions - 1) {
      subMenuNum++;
    }
  } else {
    return false;
  }

  // Leave a dial edit in progress without applying its letter.
  isSubMenuEncoderControlList = false;
  preIsSubMenuEncoderControlList = false;
  didWeJustEnteredToCharEdit = false;
  didWeJustSubmitOneCharEdit = false;
  return false;
}

bool setupPipboyNameSubmit()
{
  if (didWeJustEnteredToCharEdit && subMenuNum == subMenuItems) {
    // Click on submit button: save the name and return to the setup menu
    settingsCommit();
    setupShowMenu(0);

    return true;
  }

  return false;
}
