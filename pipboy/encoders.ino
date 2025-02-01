int32_t currentMenuEncoderPosition = 0;
int32_t previousMenuEncoderPosition = 0;
int32_t currentSubMenuEncoderPosition = 0;
int32_t previousSubMenuEncoderPosition = 0;

void encoderSetup()
{
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  menuEncoder.attachHalfQuad(MENU_ENCODER_PIN_A, MENU_ENCODER_PIN_B);
  subMenuEncoder.attachHalfQuad(SUB_MENU_ENCODER_PIN_A, SUB_MENU_ENCODER_PIN_B);

  subMenuEncoderButton.setClickHandler(onSubMenuClicked);
  subMenuEncoderButton.setLongPressHandler(onSubMenuLongPress, false);
}

void buttonsUpdate()
{
  subMenuEncoderButton.update();
}

void encoderUpdate()
{
  currentMenuEncoderPosition = menuEncoder.getCount() / 2;
  currentSubMenuEncoderPosition = subMenuEncoder.getCount() / 2;

  updateMenuByEncoder();
  updateSubMenuByEncoder();
  updateListByEncoder();

  previousMenuEncoderPosition = currentMenuEncoderPosition;
  previousSubMenuEncoderPosition = currentSubMenuEncoderPosition;
}

void onSubMenuClicked(EncoderButton& eb) {
  if (!isSubMenuEncoderControlList) {
    Serial.println("Set sub menu");
    isSubMenuEncoderControlList = true;
    return;
  }

  Serial.println("List Click");
  isSubMenuButtonClicked = true;
}

void onSubMenuLongPress(EncoderButton& eb) {
  Serial.println("Long click");
  isSubMenuEncoderControlList = false;
  isSubMenuButtonLongPressed = true;
}

void updateMenuByEncoder()
{
  if (currentMenuEncoderPosition > previousMenuEncoderPosition) {
    if (mainMenuNum < MENU_MAIN_ITEMS - 1) {
      mainMenuNum++;
      resetSubMenu();
      resetListItemMenu();
    }
  } else if (currentMenuEncoderPosition < previousMenuEncoderPosition) {
    if (mainMenuNum > 0) {
      mainMenuNum--;
      resetSubMenu();
      resetListItemMenu();
    }
  }
}

void updateSubMenuByEncoder()
{
  if (isSubMenuEncoderControlList) {
    return;
  }

  if (currentSubMenuEncoderPosition > previousSubMenuEncoderPosition) {
    if (subMenuNum < subMenuItems - 1) {
      subMenuNum++;
      resetListItemMenu();
    }
  } else if (currentSubMenuEncoderPosition < previousSubMenuEncoderPosition) {
    if (subMenuNum > 0) {
      subMenuNum--;
      resetListItemMenu();
    }
  }
}

void updateListByEncoder()
{
  if (!isSubMenuEncoderControlList) {
    return;
  }

  if (currentSubMenuEncoderPosition > previousSubMenuEncoderPosition) {
    if (listMenuNum < listMenuItems - 1) {
      listMenuNum++;
    }
  } else if (currentSubMenuEncoderPosition < previousSubMenuEncoderPosition) {
    if (listMenuNum > 0) {
      listMenuNum--;
    }
  }
}

void resetSubMenu()
{
  subMenuNum = 0;
  isSubMenuEncoderControlList = false;
}

void resetListItemMenu()
{
  listMenuNum = 0;
  isSubMenuEncoderControlList = false;
}

void resetSubMenuButton()
{
  isSubMenuButtonClicked = false;
  isSubMenuButtonLongPressed = false;
}