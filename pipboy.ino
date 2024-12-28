/**
 * Pipboy OS-ish for JC4727W543
 * Gleb Niner aka SovGVD (sovgvd@gmail.com) 2024
 */

#include <EEPROM.h>

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

/* OPTION 1: Uncomment a dev device in Arduino_GFX_dev_device.h */
#include "Arduino_GFX_dev_device.h"

#ifndef GFX_DEV_DEVICE
/* OPTION 2: Manual define hardware */

/* Step 1: Define pins in Arduino_GFX_databus.h */
#include "Arduino_GFX_pins.h"

/* Step 2: Uncomment your databus in Arduino_GFX_databus.h */
#include "Arduino_GFX_databus.h"

/* Step 3: Uncomment your display driver in Arduino_GFX_display.h */
#include "Arduino_GFX_display.h"

#endif /* Manual define hardware */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

// Load GIF library
#include <AnimatedGIF.h>
AnimatedGIF gif;

#define LOOP_TIME 33333   // microseconds (10^-6 seconds)  Idealty we should have at least 30fps that is 1/30 seconds or 33333 microseconds

#ifdef ESP32
#undef F
#define F(s) (s)
#endif

#include "fallout11.h"
#include "vaultboy.gif.h"
#include "settings.h"

char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-. ";

bool isSetup = true;

pipboySettings settings;

#define VAULT_BOY_X 170
#define VAULT_BOY_Y  60

#define STATUS_BAR_HEIGHT 30
#define STATUS_BAR_SIDE_WIDTH 120
#define STATUS_BAR_PADDING 10

#define FONT_PADDING 20

#define LEVEL_TEXT_LENGTH 110

// http://www.rinkydinkelectronics.com/calc_rgb565.php
#define COLOR_MAIN       0x45A2
#define COLOR_BACKGROUND 0x09E1
#define COLOR_INACTIVE   0x2201
#define COLOR_DEBUG      RED
#define COLOR_SCREEN     BLACK

int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1, glitchHeight;
uint8_t tsa, tsb, tsc, ds;

uint32_t stat_hp, stat_level, stat_progress, stat_ap;

uint8_t currentMenuNum = 1;

/**
 * Init structures
 */
#include "menuStructure.h"

/**
 * Initial state
 */
#define DISPLAY_WIDTH 470
#define DISPLAY_HEIGHT 270

/**
 * Number of menu item
 *  - mainMenuNum - top level menu STAT, INV...)
 *  - subMenuNum  - sub level (STAT -> Status, SPECIAL, Perks)
 *  - listMenuNum - list (STAT -> SPECIAL -> Strength, Perception, etc.)
 */
uint8_t mainMenuNum = 0;

uint8_t subMenuNum = 0;
uint8_t subMenuItems = 0;

uint16_t listMenuNum = 0;
uint16_t listMenuItems = 0;

unsigned long currentTime;
unsigned long previousTime;
unsigned long currentTick = 0;

/**
 * Menu setup
 */
#define MENU_MAIN_ITEMS 5
menuItem menuItems[MENU_MAIN_ITEMS];

#define CHAR_HEIGHT 22
#define CHAR_WIDTH  10     // mono font
#define MENU_ITEMS_PADDING 40
#define MENU_ITEMS_PADDING_SUBMENU 11
#define MENU_PADDING_TOP 1

#define MENU_HIGHLIGHT_PADDING 3
#define MENU_HIGHLIGHT_LINE 4
#define MENU_HIGHLIGHT_BOTTOM_LINE 4
#define MENU_LINE_WIDTH 2
#define MAX_LIST_MENU_ITEMS 7 // 7 means 3 on top, one selected, 3 after selected

itemGeometry menuGeometry = {0, 0, 0, 0};
itemGeometry subMenuGeometry = {0, 0, 0, 0};
itemGeometry listMenuGeometry = {0, 0, 0, 0};

/**
 * Encoders
 */
#include <ESP32Encoder.h>
#include <EncoderButton.h>
#define MENU_ENCODER_PIN_A 16
#define MENU_ENCODER_PIN_B 15
#define MENU_ENCODER_PIN_BUTTON 7

#define SUB_MENU_ENCODER_PIN_A 17
#define SUB_MENU_ENCODER_PIN_B 18
#define SUB_MENU_ENCODER_PIN_BUTTON 6

bool isSubMenuEncoderControlList = false;
bool isSubMenuButtonClicked = false;
bool isSubMenuButtonLongPressed = false;

#define MESSAGE_WINDOW_PADDING 35
#define MESSAGE_WINDOW_BORDER 5

ESP32Encoder menuEncoder;
ESP32Encoder subMenuEncoder;
EncoderButton subMenuEncoderButton(SUB_MENU_ENCODER_PIN_BUTTON);

/**
 * Radio communicatiion via ESPNOW
 */
#include <WiFi.h>
#include <esp_wifi.h>
#include <QuickEspNow.h>
#include <appRadioStructures.h>
#define USE_BROADCAST 1
#define PIPBOY_RADIO_MAX_PEERS 4

radioPeer pipboyPeers[PIPBOY_RADIO_MAX_PEERS];

/**
 * Alert/Notification/Message window
 */
bool isDisplayMessageWindow = false;
String displayMessageWindowHeader;
String displayMessageWindowBody;

/**
 * Setup device mode
 */
bool isInSetupMode = false;


void setup()
{
  Serial.begin(115200);
  Serial.println("Pipboy OS-ish loading start");

  encoderSetup();
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
  n = min(w, h);
  n1 = n - 1;
  cx = w / 2;
  cy = h / 2;
  cx1 = cx - 1;
  cy1 = cy - 1;
  cn = min(cx1, cy1);
  cn1 = cn - 1;
  tsa = ((w <= 176) || (h <= 160)) ? 1 : (((w <= 240) || (h <= 240)) ? 2 : 3); // text size A
  tsb = ((w <= 272) || (h <= 220)) ? 1 : 2;                                    // text size B
  tsc = ((w <= 220) || (h <= 220)) ? 1 : 2;                                    // text size C
  ds = (w <= 160) ? 9 : 12;                                                    // digit size

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

  gfx->setFont(&monofonto11pt7b);

  gif.begin(BIG_ENDIAN_PIXELS);

  settingsLoad();
  Serial.println("Settings... DONE");

  menuItems[0] = {"STAT",  4, {0, 0, 0, 0}, appStat};
  menuItems[1] = {"INV",   3, {0, 0, 0, 0}, appInv};
  menuItems[2] = {"DATA",  4, {0, 0, 0, 0}, appData};
  menuItems[3] = {"MAP",   3, {0, 0, 0, 0}, appMap};
  menuItems[4] = {"RADIO", 5, {0, 0, 0, 0}, appRadio};

  Serial.printf("User: %s\n", settings.pipboyName);

  if (isSubMenuButtonLongPressed) {
    setupMode();
    return;
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
  delay(1000);

  // For debug
  // setDisplayMessage("from", "message is 123");
}

void loop(void)
{
  buttonsUpdate();
  currentTime = micros();
	if (currentTime - previousTime >= LOOP_TIME) {
    encoderUpdate();
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
}

void callApp()
{
  menuItem item = getCurrentMenuItem();
  item.app();
}

/**
 * Global menu functions
 */

void drawMenu(menuItem* menu, uint8_t menuItemsNum, uint8_t selectedMenuItemNum, uint16_t color, uint16_t colorInactive, uint16_t menuXdelta)
{
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
    bool isTruncated = false;
    if (nameLength > menuMaxChars) {
      nameLength  = menuMaxChars;
      isTruncated = true;
    }

    for(uint8_t c = 0; c < menu[i].nameLength; c++) {
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

  for(uint8_t c = 0; c < stringLendgth; c++) {
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

void setDisplayMessage(char* from, char* txt)
{
  displayMessageWindowHeader = "You receive emote";
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
  displayTxt(displayMessageWindowBody, MESSAGE_WINDOW_PADDING + MESSAGE_WINDOW_BORDER * 2, MESSAGE_WINDOW_PADDING + CHAR_HEIGHT * 3, 20);

  if (isSubMenuButtonClicked || isSubMenuButtonLongPressed) {
    isDisplayMessageWindow = false;
  }
}
