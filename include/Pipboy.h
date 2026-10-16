#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <AnimatedGIF.h>
#include <ESP32Encoder.h>
#include <EncoderButton.h>
#include "settings.h"
#include "menuStructure.h"
#include "appRadioStructures.h"
#include "Hardware.h"

#define LOOP_TIME 33333UL
#define VAULT_BOY_X 170
#define VAULT_BOY_Y 60
#define STATUS_BAR_HEIGHT 30
#define STATUS_BAR_SIDE_WIDTH 120
#define STATUS_BAR_PADDING 10
#define FONT_PADDING 20
#define LEVEL_TEXT_LENGTH 110
#define COLOR_MAIN 0x45A2
#define COLOR_BACKGROUND 0x09E1
#define COLOR_INACTIVE 0x2201
#define COLOR_DEBUG RED
#define COLOR_SCREEN BLACK
#define DISPLAY_WIDTH 470
#define DISPLAY_HEIGHT 270
#define MENU_MAIN_ITEMS 5
#define CHAR_HEIGHT 22
#define CHAR_WIDTH 10
#define MENU_ITEMS_PADDING 40
#define MENU_ITEMS_PADDING_SUBMENU 11
#define MENU_PADDING_TOP 1
#define MENU_HIGHLIGHT_PADDING 3
#define MENU_HIGHLIGHT_LINE 4
#define MENU_HIGHLIGHT_BOTTOM_LINE 4
#define MENU_LINE_WIDTH 2
#define MAX_LIST_MENU_ITEMS 7
#define MENU_ENCODER_PIN_A 16
#define MENU_ENCODER_PIN_B 15
#define MENU_ENCODER_PIN_BUTTON 7
#define SUB_MENU_ENCODER_PIN_A 17
#define SUB_MENU_ENCODER_PIN_B 18
#define SUB_MENU_ENCODER_PIN_BUTTON 6
#define PIPBOY_RADIO_MAX_PEERS 4
#define MESSAGE_WINDOW_PADDING 35
#define MESSAGE_WINDOW_BORDER 5

extern Arduino_GFX* gfx;
extern AnimatedGIF gif;
extern char allowedChars[];
extern bool isSetup;
extern pipboySettings settings;
extern int32_t w, h;
extern uint32_t stat_hp, stat_level, stat_progress, stat_ap;
extern uint8_t mainMenuNum, subMenuNum, subMenuItems;
extern uint16_t listMenuNum, listMenuItems;
extern unsigned long currentTime, previousTime, currentTick;
extern menuItem menuItems[MENU_MAIN_ITEMS];
extern itemGeometry menuGeometry, subMenuGeometry, listMenuGeometry;
extern ESP32Encoder menuEncoder, subMenuEncoder;
extern EncoderButton subMenuEncoderButton;
extern bool isSubMenuEncoderControlList, isSubMenuButtonClicked, isSubMenuButtonLongPressed;
extern radioPeer pipboyPeers[PIPBOY_RADIO_MAX_PEERS];
extern bool isDisplayMessageWindow, isInSetupMode;
extern String displayMessageWindowHeader, displayMessageWindowBody;

// Explicit declarations replace Arduino IDE generated prototypes.
void setupAppRadioMenu();
void appRadio();
void appEmotes();
void appRadioEmoteSelector();
void appRadioPeerSelector();
void appRadioNoSignal();
void appRadioPeerItem();
void setupRadio();
void radioDataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
bool radioIsForThisPipboy(char* to);
void processRadioMessage(char* cmd, char* from, char* to, char* body, signed int rssi);
void processRadioMessageEmote(char* from, char* emote);
void radioRegisterOrUpdateClient(char* peerName, signed int rssi);
void radioUpdatePipboyPeersCount();
void radioUpdatePeerOnlineStatus(uint8_t peerIndex);
radioMsg radioMessage(const char* cmd, char* to, char* body, bool isBroadcast);
void radioPing();
void radioSend(radioMsg msg);
void radioUpdate();
void radioSendEmote(const char* emoteCode);
void radioShowPreSelectedEmote(const char* emoteCode, const char* txt);
void appRadioEmotesConfused();
void appRadioEmotesFingerGuns();
void appRadioEmotesFollow();
void appRadioEmotesFrown();
void appRadioEmotesHeart();
void appRadioEmotesJoinMyTeam();
void appRadioEmotesRadiated();
void appRadioEmotesStartled();
void appRadioEmotesThumbsDown();
void appRadioEmotesThumbsUp();
void appRadioEmotesWave();
void appRadioEmotesFear();
void appPlayer();
void setupMode();
void setupLoop();
void setupPipboyName();
bool setupPipboyNameSubmit();
void encoderSetup();
void buttonsUpdate();
void encoderUpdate();
void onSubMenuClicked(EncoderButton& eb);
void onSubMenuLongPress(EncoderButton& eb);
void updateMenuByEncoder();
void updateSubMenuByEncoder();
void updateListByEncoder();
void resetSubMenu();
void resetListItemMenu();
void resetSubMenuButton();
void setupStat();
void setupStatMenu();
void appStat();
void appStatus();
void appStatBar();
void setupStatSpecial();
void appSpecial();
void appStatSpecialStrength();
void appStatSpecialPerception();
void appStatSpecialEndurance();
void appStatSpecialCharisma();
void appStatSpecialIntelligence();
void appStatSpecialAgility();
void appStatSpecialLuck();
void appPerks();
void setupInvMenu();
void appInv();
void appInvWeapons();
void appInvApparel();
void appInvAid();
void appInvMisc();
void appInvJunk();
void appData();
void appMap();
void settingsLoad();
void settingsCheck();
void settingsInitEEPROM();
void settingsSanitize();
void settingsSanitizeClearSpaces();
void settingsCommit();
void setup();
void loop(void);
void callApp();
void drawMenu(menuItem* menu, uint8_t menuItemsNum, uint8_t selectedMenuItemNum, uint16_t color, uint16_t colorInactive, uint16_t menuXdelta);
void updateMenuGeometry(menuItem* menu, uint8_t menuItemsNum, uint16_t menuPadding, itemGeometry givenMenuGeometry);
void drawListMenu(menuItem* menu, uint8_t menuItemsNum, uint8_t selectedMenuItemNum, uint16_t color, uint16_t colorBackground, uint16_t menuX, uint16_t menuY, uint8_t menuMaxChars);
void displayTxt(String txt, uint16_t x, uint16_t y, uint16_t wMaxChars);
void setDisplayMessage(const char* from, const char* txt);
void displayMessage();
void GIFDraw(GIFDRAW *pDraw);
void setupMenu();
void setupSubMenu(menuItem* menu, uint8_t menuItemsNum);
menuItem getCurrentMenuItem();
void drawMainMenu();
void drawMainMenuHighlights(uint16_t color);
