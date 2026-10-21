// Pipboy OS-ish for JC4827W543
// Gleb Niner aka SovGVD (sovgvd@gmail.com), 2024
#include "Pipboy.h"
#include "Hardware.h"
#include "MusicPlayer.h"
#include "assets/fallout11.h"

AnimatedGIF gif;
char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-. ";
bool isSetup = true;
pipboySettings settings;
int32_t w, h;
uint32_t stat_hp, stat_level, stat_progress, stat_ap;
uint8_t mainMenuNum = 0, subMenuNum = 0, subMenuItems = 0;
uint16_t listMenuNum = 0, listMenuItems = 0;
unsigned long currentTime = 0, previousTime = 0, currentTick = 0;
menuItem menuItems[MENU_MAIN_ITEMS];
itemGeometry menuGeometry = {}, subMenuGeometry = {}, listMenuGeometry = {};
ESP32Encoder menuEncoder, subMenuEncoder;
EncoderButton subMenuEncoderButton(SUB_MENU_ENCODER_PIN_BUTTON);
bool isSubMenuEncoderControlList = false;
bool isSubMenuButtonClicked = false;
bool isSubMenuButtonLongPressed = false;
radioPeer pipboyPeers[PIPBOY_RADIO_MAX_PEERS] = {};
bool isDisplayMessageWindow = false;
String displayMessageWindowHeader, displayMessageWindowBody;
bool isInSetupMode = false;
bool isTouchTapped = false, isTouchDown = false;
int16_t touchX = -1, touchY = -1, touchDownX = -1, touchDownY = -1;

namespace {
bool isBootButtonDown = false, isBootButtonHoldReported = false, isBootButtonClickPending = false;
bool isBootButtonHoldPending = false;
unsigned long bootButtonDownAt = 0;
}

/**
 * BOOT works without the dials: a short press reports a click, holding it
 * for bootButtonHoldMs reports a hold (once per press).
 */
void bootButtonUpdate()
{
  const bool isDown = digitalRead(hardware::bootButton) == LOW;
  const unsigned long now = millis();
  if (isDown && !isBootButtonDown) {
    bootButtonDownAt = now;
    isBootButtonHoldReported = false;
  }
  if (isDown && !isBootButtonHoldReported && now - bootButtonDownAt >= hardware::bootButtonHoldMs) {
    isBootButtonHoldReported = true;
    isBootButtonHoldPending = true;
  }
  if (!isDown && isBootButtonDown && !isBootButtonHoldReported) {
    isBootButtonClickPending = true;
  }
  isBootButtonDown = isDown;
}

bool bootButtonClicked()
{
  const bool isClicked = isBootButtonClickPending;
  isBootButtonClickPending = false;
  return isClicked;
}

bool bootButtonHeld()
{
  const bool isHeld = isBootButtonHoldPending;
  isBootButtonHoldPending = false;
  return isHeld;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Pipboy OS-ish loading start");

  encoderSetup();
  pinMode(hardware::bootButton, INPUT_PULLUP);
  Serial.println("Encoders... DONE");

  buttonsUpdate();
  Serial.println("Buttons... DONE");

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  // if (!gfx->begin(80000000)) /* specify data bus speed */
  {
    Serial.println("gfx->begin() failed!");
  }

  w = gfx->width();
  h = gfx->height();
#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->fillScreen(COLOR_SCREEN);

  gfx->setTextWrap(false);

  gfx->setCursor(10, 20);
  gfx->println("Loading...");
  #ifdef CANVAS
    gfx->flush();
  #endif

  setUiFont();

  gif.begin(BIG_ENDIAN_PIXELS);

  settingsLoad();
  Serial.println("Settings... DONE");

  // Before setup mode and before the music task mounts SD on the shared bus.
  touchSetup();

  menuItems[0] = menuItem{"STAT",  4, {0, 0, 0, 0}, appStat};
  menuItems[1] = menuItem{"INV",   3, {0, 0, 0, 0}, appInv};
  menuItems[2] = menuItem{"DATA",  4, {0, 0, 0, 0}, appData};
  menuItems[3] = menuItem{"MAP",   3, {0, 0, 0, 0}, appMap};
  menuItems[4] = menuItem{"RADIO", 5, {0, 0, 0, 0}, appRadio};

  Serial.printf("User: %s\n", settings.pipboyName);

  // Setup without the dials: a finger held on the screen while booting.
  if (isSubMenuButtonLongPressed || touchIsHeld()) {
    setupMode();
    return;
  }

  if (!touchHasSavedCalibration()) {
    // First start: calibrate once; skipping keeps the defaults.
    touchCalibrate(true);
  }

  stat_hp = random(30, 99);
  stat_level = random(10,51);
  stat_progress = random(30, 60);
  stat_ap = random(40, 70);

  Serial.println("menuSetup...");
  setupMenu();
  Serial.println("setupStat...");
  setupStat();
  Serial.println("setupRadio...");
  setupRadio();

  Serial.println("resetButtons...");
  resetSubMenuButton();
  Serial.println("All DONE!");
  isSetup = false;
  music::begin();

  // For debug
  // setDisplayMessage("Incoming message", "from", "message is 123");
}

void loop(void)
{
  buttonsUpdate();
  bootButtonUpdate();
  if (bootButtonHeld()) {
    setupMode(); // restarts when done
  }
  currentTime = micros();
	if (currentTime - previousTime >= LOOP_TIME) {
    encoderUpdate();
    touchUpdate();
    touchDispatch();
    previousTime = currentTime;
    gfx->fillScreen(COLOR_SCREEN);
    radioUpdate();
    drawMainMenu();
    callApp();
    displayMessage();
    // gfx->setCursor(10,20);
    // gfx->setTextColor(COLOR_DEBUG);
    // gfx->print(currentTick/30);
    #ifdef CANVAS
      gfx->flush();
    #endif
    currentTick++;
    resetSubMenuButton();
  }
  delay(1);
}

void callApp()
{
  subMenuItems = 0;
  listMenuItems = 0;
  menuItem item = getCurrentMenuItem();
  item.app();
}

void setUiFont() {
  gfx->setFont(&monofonto11pt7b);
}

/**
 * Global menu functions
 */

void drawMenu(menuItem* menu, uint8_t menuItemsNum, uint8_t selectedMenuItemNum, uint16_t color, uint16_t colorInactive, uint16_t menuXdelta)
{
  if (menu != menuItems) {
    touchRegisterSubMenu(menu, menuItemsNum, menuXdelta);
  }

  for(uint8_t i = 0; i < menuItemsNum; i++) {
    if (i == selectedMenuItemNum) {
      gfx->setTextColor(color);
    } else {
      gfx->setTextColor(colorInactive);
    }
    for(uint8_t c = 0; c < menu[i].nameLength; c++) {
      gfx->setCursor(menu[i].g.x + c * CHAR_WIDTH + menuXdelta, menu[i].g.y + menu[i].g.h - 1);  // We add height because font render from baseline, that is kind of left bottom and extra pixel
      gfx->print(menu[i].name[c]);
    }
  }
}

void updateMenuGeometry(menuItem* menu, uint8_t menuItemsNum, uint16_t menuPadding, itemGeometry givenMenuGeometry)
{
  for(uint8_t i = 0; i < menuItemsNum; i++) {
    menu[i].g.w = menu[i].nameLength * CHAR_WIDTH;
    menu[i].g.h = CHAR_HEIGHT;
    menu[i].g.y = givenMenuGeometry.y - 1;
    menu[i].g.x = (i == 0
                ? givenMenuGeometry.x
                : (menuPadding + menu[i-1].g.x + menu[i-1].g.w)
                );
  }
}

void drawListMenu(menuItem* menu, uint8_t menuItemsNum, uint8_t selectedMenuItemNum, uint16_t color, uint16_t colorBackground, uint16_t menuX, uint16_t menuY, uint8_t menuMaxChars)
{
  if (!menuItemsNum) return;
  selectedMenuItemNum = min(selectedMenuItemNum, static_cast<uint8_t>(menuItemsNum - 1));
  uint16_t menuMiddle = ((MAX_LIST_MENU_ITEMS - 1) / 2);
  bool isMenuTruncated = menuItemsNum > MAX_LIST_MENU_ITEMS;

  uint16_t startFrom = 0;
  uint16_t endAt = menuItemsNum - 1;
  if (isMenuTruncated) {
    if (selectedMenuItemNum >= menuMiddle) {
      if (menuItemsNum - selectedMenuItemNum - 1 >= menuMiddle) {
        startFrom = selectedMenuItemNum - menuMiddle;
        endAt = selectedMenuItemNum + menuMiddle;
      } else {
        startFrom = endAt - MAX_LIST_MENU_ITEMS + 1;
      }
    } else {
      endAt = MAX_LIST_MENU_ITEMS - 1;
    }
  }

  touchRegisterList(menuX - 2, menuY - CHAR_HEIGHT + 4, menuMaxChars * CHAR_WIDTH + 5, CHAR_HEIGHT, startFrom, endAt - startFrom + 1);

  for(uint16_t i = startFrom; i <= endAt; i++) {
    if (i == selectedMenuItemNum) {
      gfx->fillRect(
        menuX - 2,                     menuY + CHAR_HEIGHT * (i - startFrom - 1) + 4,
        menuMaxChars * CHAR_WIDTH + 5, CHAR_HEIGHT,
        color
      );
      gfx->setTextColor(colorBackground);
    } else {
      gfx->setTextColor(color);
    }
    uint16_t nameLength = menu[i].nameLength;
    if (nameLength > menuMaxChars) {
      nameLength  = menuMaxChars;
    }

    for(uint8_t c = 0; c < nameLength; c++) {
      gfx->setCursor(menuX + c * CHAR_WIDTH, menuY + CHAR_HEIGHT * (i - startFrom));
      gfx->print(menu[i].name[c]);
    }
  }
}

void displayTxt(String txt, uint16_t x, uint16_t y, uint16_t wMaxChars)
{
  gfx->setTextColor(COLOR_MAIN);
  uint16_t stringLendgth = txt.length();
  uint16_t currentStringLength = 0;
  uint16_t currentLine = 0;

  for(uint16_t c = 0; c < stringLendgth; c++) {
    char symbol = txt.charAt(c);
    if (currentStringLength == 0 && symbol == 32) {
      continue;
    }
    gfx->setCursor(x + CHAR_WIDTH * currentStringLength, y + CHAR_HEIGHT * currentLine);
    gfx->print(symbol);
    if (currentStringLength >= wMaxChars || symbol == 10) {
      if (symbol == 32 || symbol == 44 || symbol == 46 || symbol == 10) { // Break only on space, comma or period or just new line(unix), @TODO add more
        currentStringLength = 0;
        currentLine++;
        continue;
      }
    }

    currentStringLength++;
  }
}

void setDisplayMessage(const char* header, const char* from, const char* txt)
{
  displayMessageWindowHeader = header;
  displayMessageWindowBody = "From: " + (String)from + "\n" + (String)txt;

  isDisplayMessageWindow = true;
}

void displayMessage()
{
  if (!isDisplayMessageWindow) {
    return;
  }

  // Draw shadow
  gfx->fillRect(
    MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER, MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER,
    DISPLAY_WIDTH - MESSAGE_WINDOW_PADDING * 2, DISPLAY_HEIGHT - MESSAGE_WINDOW_PADDING * 2,
    COLOR_INACTIVE
  );

  // Draw kind of window
  gfx->fillRect(
    MESSAGE_WINDOW_PADDING, MESSAGE_WINDOW_PADDING,
    DISPLAY_WIDTH - MESSAGE_WINDOW_PADDING * 2, DISPLAY_HEIGHT - MESSAGE_WINDOW_PADDING * 2,
    COLOR_MAIN
  );

  // Clear space for text
  gfx->fillRect(
    MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER, MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER,
    DISPLAY_WIDTH - (MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER) * 2, DISPLAY_HEIGHT - (MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER) * 2,
    COLOR_SCREEN
  );

  // Line to separate header and body
  gfx->fillRect(
    MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER, MESSAGE_WINDOW_PADDING + CHAR_HEIGHT * 1.4,
    DISPLAY_WIDTH - (MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER) * 2, MESSAGE_WINDOW_BORDER,
    COLOR_MAIN
  );

  displayTxt(displayMessageWindowHeader, MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER * 2, MESSAGE_WINDOW_PADDING + CHAR_HEIGHT, 20);
  // Chat text arrives pre-wrapped to 34 columns, so only its new lines break it.
  displayTxt(displayMessageWindowBody, MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER * 2, MESSAGE_WINDOW_PADDING + CHAR_HEIGHT * 3, 36);

  if (isSubMenuButtonClicked || isSubMenuButtonLongPressed || isTouchTapped) {
    isDisplayMessageWindow = false;
  }
}
