bool didWeJustEnteredToCharEdit = false;
bool didWeJustSubmitOneCharEdit = false;
bool preIsSubMenuEncoderControlList = false;

void setupMode()
{
  Serial.println("Setup mode");
  isInSetupMode = true;
  subMenuNum = 0;
  listMenuNum = 0;
  isSubMenuEncoderControlList = false;
  
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


      setupPipboyName();


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
  }
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

  subMenuItems = sizeof(settings.pipboyName);

  if (setupPipboyNameSubmit()) {
    return;
  }

  if (didWeJustEnteredToCharEdit) {
    if (subMenuNum == subMenuItems) {
      // Click on submit button
      settingsCommit();
      isInSetupMode = false;

      return;
    }

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
    // Click on submit button
    settingsCommit();
    isInSetupMode = false;

    return true;
  }

  return false;
}
