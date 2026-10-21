#include "Pipboy.h"
bool didWeJustEnteredToCharEdit = false;
bool didWeJustSubmitOneCharEdit = false;
bool preIsSubMenuEncoderControlList = false;

enum class SetupPage : uint8_t { Menu, Name };
SetupPage setupPage = SetupPage::Menu;

#define SETUP_MENU_ITEMS 3
#define SETUP_MENU_Y 87

menuItem setupModeMenu[SETUP_MENU_ITEMS] = {
  {"CHANGE NAME",     11, {0, 0, 0, 0}, setupMenuChangeName},
  {"CALIBRATE TOUCH", 15, {0, 0, 0, 0}, setupMenuCalibrateTouch},
  {"SAVE AND EXIT",   13, {0, 0, 0, 0}, setupMenuExit},
};

void setupMode()
{
  Serial.println("Setup mode");
  isInSetupMode = true;
  setupShowMenu(0);

  setupLoop();

  Serial.println("Setup loop done... Restart");
  ESP.restart();
}

void setupLoop() {
  while(isInSetupMode) {
    buttonsUpdate();
    currentTime = micros();
    if (currentTime - previousTime >= LOOP_TIME) {
      encoderUpdate();
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

  if (setupPipboyNameSubmit()) {
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
    settings.pipboyName[subMenuNum] = allowedChars[listMenuNum];
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

  subMenuItems++; // For [SUBMIT] button
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
