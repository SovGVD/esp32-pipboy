#include <WiFi.h>
#include <QuickEspNow.h>
#include "MusicPlayer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Pipboy.h"
const char* PIPBOY_RADIO_DELIMETER = "\t";  // tab
const char* PIPBOY_RADIO_HEADER = "PIPBOY";
const char* PIPBOY_RADIO_PING = "P";
const char* PIPBOY_RADIO_EMOTE = "E"; // Emote

const char* PIPBOY_RADIO_EMPTY = "_";

const char* PIPBOY_RADIO_EMOTE_CONFUSED = "confused";
const char* PIPBOY_RADIO_EMOTE_FINREDGU = "fingergu";
const char* PIPBOY_RADIO_EMOTE_FOLLOW   = "follow";
const char* PIPBOY_RADIO_EMOTE_FROWN    = "frown";
const char* PIPBOY_RADIO_EMOTE_HEART    = "heart";
const char* PIPBOY_RADIO_EMOTE_JOINMYTE = "joinmyte";
const char* PIPBOY_RADIO_EMOTE_RADIATED = "radiated";
const char* PIPBOY_RADIO_EMOTE_STARTLED = "startled";
const char* PIPBOY_RADIO_EMOTE_THDOWN   = "thdown";
const char* PIPBOY_RADIO_EMOTE_THUP     = "thup";
const char* PIPBOY_RADIO_EMOTE_WAVE     = "wave";
const char* PIPBOY_RADIO_EMOTE_FEAR     = "fear";

uint8_t pipboyPeersCount = 0;
bool isPeerSelected = false;
uint8_t selectedPeerIdx = 0;
unsigned long offlineAfter;
const unsigned long pipboyPeerTTL = 150; // 150 ticks is about 5 second (1 tick per frame)
const unsigned long PIPBOY_RADIO_PING_TTL = 3000000; // microseconds (10^-6 seconds)

unsigned long previousPingTime;

#define APP_RADIO_PEERS_Y 87
#define PIPBOY_RADIO_UNKNOWN_PEER 255

#define APP_RADIO_EMOTE_X 200
#define APP_RADIO_EMOTE_Y 87
#define APP_RADIO_EMOTE_W 20

#define APP_RADIO_MENU_ITEMS 2
bool isRadioMenuReady = false;

menuItem appRadioMenu[APP_RADIO_MENU_ITEMS] = {
  {"MUSIC",  5, {0, 0, 0, 0}, appPlayer},
  {"EMOTES", 6, {0, 0, 0, 0}, appEmotes},
};

#define APP_RADIO_EMOTES_MENU_ITEMS 12

// @src https://fallout.fandom.com/wiki/Atomic_Shop/Emotes
menuItem appRadioEmotes[APP_RADIO_EMOTES_MENU_ITEMS] = {
  {"Confused",        8, {0, 0, 0, 0}, appRadioEmotesConfused},
  {"Finger Guns",    11, {0, 0, 0, 0}, appRadioEmotesFingerGuns},
  {"Follow!",         7, {0, 0, 0, 0}, appRadioEmotesFollow},
  {"Frown",           5, {0, 0, 0, 0}, appRadioEmotesFrown},
  {"Heart",           5, {0, 0, 0, 0}, appRadioEmotesHeart},
  {"Join My Team!",  13, {0, 0, 0, 0}, appRadioEmotesJoinMyTeam},
  {"Radiated",        8, {0, 0, 0, 0}, appRadioEmotesRadiated},
  {"Startled",        8, {0, 0, 0, 0}, appRadioEmotesStartled},
  {"Thumbs Down",    11, {0, 0, 0, 0}, appRadioEmotesThumbsDown},
  {"Thumbs Up",       9, {0, 0, 0, 0}, appRadioEmotesThumbsUp},
  {"Wave",            4, {0, 0, 0, 0}, appRadioEmotesWave},
  {"Fear",            4, {0, 0, 0, 0}, appRadioEmotesFear},
};

void setupAppRadioMenu()
{
  setupSubMenu(appRadioMenu, APP_RADIO_MENU_ITEMS);
  isRadioMenuReady = true;
}

void appRadio()
{
  if (!isRadioMenuReady) {
    setupAppRadioMenu();
  }

  subMenuItems = APP_RADIO_MENU_ITEMS;

  drawMenu(
    appRadioMenu,
    APP_RADIO_MENU_ITEMS,
    subMenuNum,
    COLOR_MAIN,
    COLOR_INACTIVE,
    menuItems[4].g.x - appRadioMenu[subMenuNum].g.x
  );

  appRadioMenu[subMenuNum].app();
}

/**
 * /!\ Please don't expect any security/privacy/crtypto features, that is as simple as possible /!\
 */
void appEmotes()
{
  radioUpdatePipboyPeersCount();

  if (pipboyPeersCount == 0) {
    appRadioNoSignal();
    return;
  }

  if (isPeerSelected && !pipboyPeers[selectedPeerIdx].isOnline) {
    isPeerSelected = false;
    listMenuNum = 0;
  }

  if (isPeerSelected) {
    appRadioEmoteSelector();
  } else {
    appRadioPeerSelector();
  }
}

void appRadioEmoteSelector()
{
  listMenuItems = APP_RADIO_EMOTES_MENU_ITEMS;

  drawListMenu(
    appRadioEmotes,
    APP_RADIO_EMOTES_MENU_ITEMS,
    listMenuNum,
    COLOR_MAIN,
    COLOR_SCREEN,
    20,
    APP_RADIO_PEERS_Y,
    16
  );

  appRadioEmotes[listMenuNum].app();
}

void appRadioPeerSelector()
{
  /**
   * @TODO broadcast option
   */
  uint8_t menuIdx = 0;
  menuItem peers[PIPBOY_RADIO_MAX_PEERS] = {};
  if (listMenuNum >= pipboyPeersCount) listMenuNum = pipboyPeersCount - 1;

  for (uint8_t idx = 0; idx < PIPBOY_RADIO_MAX_PEERS; idx++) {
    if (!pipboyPeers[idx].isOnline) {
      continue;
    }

    strncpy(peers[menuIdx].name, pipboyPeers[idx].name, pipboyPeers[idx].nameLength);
    peers[menuIdx].name[pipboyPeers[idx].nameLength] = '\0';

    peers[menuIdx].nameLength = pipboyPeers[idx].nameLength;
    peers[menuIdx].g = {0,0,0,0};
    peers[menuIdx].app = appRadioPeerItem;  // Do nothing

    if (listMenuNum == menuIdx) {
      selectedPeerIdx = idx;
    }

    menuIdx++;
  }

  listMenuItems = pipboyPeersCount;

  drawListMenu(
    peers,
    pipboyPeersCount,
    listMenuNum,
    COLOR_MAIN,
    COLOR_SCREEN,
    20,
    APP_RADIO_PEERS_Y,
    16
  );

  if (isSubMenuButtonClicked) {
    peers[listMenuNum].app();
    listMenuNum = 0;
  }
}

void appRadioNoSignal()
{
  gfx->setCursor(20, APP_RADIO_PEERS_Y);
  gfx->setTextColor(COLOR_MAIN);
  gfx->print("NO SIGNAL");
}

void appRadioPeerItem()
{
  listMenuNum = 0;
  isPeerSelected = true;
}

namespace {
struct ReceivedPacket {
  char body[127];
  int rssi;
};
QueueHandle_t receivedPackets = nullptr;
}

void setupRadio()
{
  receivedPackets = xQueueCreate(4, sizeof(ReceivedPacket));
  if (!receivedPackets) {
    Serial.println("Cannot create ESP-NOW receive queue");
    return;
  }
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect(false, true);
  Serial.printf ("Connected to %s in channel %d\n", WiFi.SSID ().c_str (), WiFi.channel ());
  Serial.printf ("IP address: %s\n", WiFi.localIP ().toString ().c_str ());
  Serial.printf ("MAC address: %s\n", WiFi.macAddress ().c_str ());
  quickEspNow.begin (1, 0, false);
  quickEspNow.onDataRcvd(radioDataReceived);
}

void radioDataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast)
{
  // WiFi callback runs on another task. Validate and copy exactly len bytes;
  // parsing and all peer/UI updates happen later on the main loop.
  if (!receivedPackets || !data || !len || len >= sizeof(ReceivedPacket::body) ||
      memchr(data, '\0', len)) return;
  ReceivedPacket packet = {};
  memcpy(packet.body, data, len);
  packet.rssi = rssi;
  xQueueSend(receivedPackets, &packet, 0);
}


bool radioIsForThisPipboy(char* to)
{
  if (strlen(to) == 1 && strcmp(to, PIPBOY_RADIO_EMPTY) == 0) {
    // This is broadcast
    Serial.printf("DBG: [%s] this is broadcast\n", to);
    return true;
  }

  return strcmp(to, settings.pipboyName) == 0;
}

void processRadioMessage(char* cmd, char* from, char* to, char* body, signed int rssi)
{
  // Ping for everyone
  if (strcmp(cmd, PIPBOY_RADIO_PING) == 0) {
    radioRegisterOrUpdateClient(from, rssi);
    return;
  }

  // Message is not for that pipboy (is not broadcast)
  if (!radioIsForThisPipboy(to)) {
    Serial.printf("ERR: [%s] not for this pipboy, expected [%s]\n", to, settings.pipboyName);
    return;
  }

  if (strcmp(cmd, PIPBOY_RADIO_EMOTE) == 0) {
    processRadioMessageEmote(from, body);
    return;
  }
}

void processRadioMessageEmote(char* from, char* emote)
{
  const char* peerName = "UNKNOWN";
  for (uint8_t idx = 0; idx < PIPBOY_RADIO_MAX_PEERS; idx++) {
    if (strlen(pipboyPeers[idx].name) == strlen(from) && memcmp(pipboyPeers[idx].name, from, strlen(from)) == 0) {
      peerName = (char*) pipboyPeers[idx].name;
    }
  }

  // @TODO display GIFs?
  // @TODO that copy-paste killing me
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_CONFUSED) == 0) {
    setDisplayMessage(peerName, "Confused");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_FINREDGU) == 0) {
    setDisplayMessage(peerName, "Finger Guns");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_FOLLOW) == 0) {
    setDisplayMessage(peerName, "Follow!");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_FROWN) == 0) {
    setDisplayMessage(peerName, "Frown");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_HEART) == 0) {
    setDisplayMessage(peerName, "Heart");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_JOINMYTE) == 0) {
    setDisplayMessage(peerName, "Join My Team!");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_RADIATED) == 0) {
    setDisplayMessage(peerName, "Radiated");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_STARTLED) == 0) {
    setDisplayMessage(peerName, "Startled");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_THDOWN) == 0) {
    setDisplayMessage(peerName, "Thumbs Down");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_THUP) == 0) {
    setDisplayMessage(peerName, "Thumbs Up");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_WAVE) == 0) {
    setDisplayMessage(peerName, "Wave");
  }
  if (strcmp(emote, PIPBOY_RADIO_EMOTE_FEAR) == 0) {
    setDisplayMessage(peerName, "Fear");
  }
}


void radioRegisterOrUpdateClient(char* peerName, signed int rssi)
{
  if (PIPBOY_RADIO_MAX_PEERS > 200) {
    Serial.println("Cant add for such amount of peers");
    return;
  }

  const size_t nameLength = strlen(peerName);
  if (!nameLength || nameLength >= sizeof(pipboyPeers[0].name)) {
    Serial.println("Peer name is too long");
    return;
  }

  Serial.printf("New client [%s,%d]...", peerName, strlen(peerName));

  uint8_t peerIndex = PIPBOY_RADIO_UNKNOWN_PEER;
  bool isExiting = false;

  for (uint8_t idx = 0; idx < PIPBOY_RADIO_MAX_PEERS; idx++) {
    if (strlen(pipboyPeers[idx].name) == strlen(peerName) && memcmp(pipboyPeers[idx].name, peerName, strlen(peerName)) == 0) {
      Serial.print("existing...");
      peerIndex = idx;
      isExiting = true;
      break;
    }

    if (peerIndex == PIPBOY_RADIO_UNKNOWN_PEER && !pipboyPeers[idx].isOnline) {
      peerIndex = idx;
      isExiting = false;
    }
  }

  if (peerIndex == PIPBOY_RADIO_UNKNOWN_PEER) {
    // Too much peers
    return;
  }

  if (!isExiting) {
    strncpy(pipboyPeers[peerIndex].name, peerName, strlen(peerName));
    pipboyPeers[peerIndex].name[nameLength] = '\0';
    pipboyPeers[peerIndex].nameLength = nameLength;
  }

  pipboyPeers[peerIndex].rssi = rssi;
  pipboyPeers[peerIndex].lastTickOnline = currentTick;
  pipboyPeers[peerIndex].isOnline = true;
  pipboyPeers[peerIndex].isBroadcast = false;

  if (isExiting) {
    Serial.println("existing client updated");
  } else {
    Serial.println("new client added");
  }
}

void radioUpdatePipboyPeersCount()
{
  pipboyPeersCount = 0;
  for (uint8_t idx = 0; idx < PIPBOY_RADIO_MAX_PEERS; idx++) {
    radioUpdatePeerOnlineStatus(idx);
    if (pipboyPeers[idx].isOnline) {
      pipboyPeersCount++;
    }
  }
}

void radioUpdatePeerOnlineStatus(uint8_t peerIndex)
{
  if (!pipboyPeers[peerIndex].isOnline) {
    return;
  }

  if (pipboyPeerTTL > currentTick) {
    return;
  }

  offlineAfter = currentTick - pipboyPeerTTL;

  if (pipboyPeers[peerIndex].lastTickOnline < offlineAfter) {
    pipboyPeers[peerIndex].isOnline = false;
  }
}

radioMsg radioMessage(const char* cmd, char* to, char* body, bool isBroadcast)
{
  radioMsg sMsg = {};
  const int length = snprintf(sMsg.body, sizeof(sMsg.body), "%s\t%s\t%s\t%s\t%s",
    PIPBOY_RADIO_HEADER, settings.pipboyName,
    isBroadcast ? PIPBOY_RADIO_EMPTY : to, cmd, body);
  if (length > 0 && static_cast<size_t>(length) < sizeof(sMsg.body)) sMsg.len = length;

  return sMsg;
}

void radioPing()
{
  if (currentTime - previousPingTime >= PIPBOY_RADIO_PING_TTL) {
    radioMsg pingMessage = radioMessage(PIPBOY_RADIO_PING, (char*)PIPBOY_RADIO_EMPTY, (char*)PIPBOY_RADIO_EMPTY, true);
    radioSend(pingMessage);
    previousPingTime = currentTime;
  }
}

void radioSend(radioMsg msg)
{
  if (!msg.len) return;
  Serial.printf("Sending:[%s](%d)\n", msg.body, msg.len);

  if (quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, (uint8_t*)msg.body, msg.len)) {
      Serial.println (">>>>>>>>>> Message not sent");
  }
}

void radioUpdate()
{
  ReceivedPacket packet;
  while (receivedPackets && xQueueReceive(receivedPackets, &packet, 0) == pdTRUE) {
    char* save = nullptr;
    char* header = strtok_r(packet.body, PIPBOY_RADIO_DELIMETER, &save);
    char* from = strtok_r(nullptr, PIPBOY_RADIO_DELIMETER, &save);
    char* to = strtok_r(nullptr, PIPBOY_RADIO_DELIMETER, &save);
    char* cmd = strtok_r(nullptr, PIPBOY_RADIO_DELIMETER, &save);
    char* body = strtok_r(nullptr, PIPBOY_RADIO_DELIMETER, &save);
    if (!header || !from || !to || !cmd || !body ||
        strcmp(header, PIPBOY_RADIO_HEADER) || strlen(from) >= sizeof(radioPeer::name) ||
        strlen(to) >= sizeof(radioPeer::name)) continue;
    processRadioMessage(cmd, from, to, body, packet.rssi);
  }
  radioPing();
}

void radioSendEmote(const char* emoteCode)
{
  if (!isSubMenuButtonClicked) {
    return;
  }

  radioMsg emoteMessage = radioMessage(
    PIPBOY_RADIO_EMOTE,
    pipboyPeers[selectedPeerIdx].name,
    (char*) emoteCode,
    pipboyPeers[selectedPeerIdx].isBroadcast
  );

  radioSend(emoteMessage);

  isPeerSelected = false;
  listMenuNum = 0;
}

void radioShowPreSelectedEmote(const char* emoteCode, const char* txt)
{
  displayTxt(txt, APP_RADIO_EMOTE_X, APP_RADIO_EMOTE_Y, APP_RADIO_EMOTE_W);

  if (isSubMenuButtonClicked) {
    radioSendEmote(emoteCode);
  }
}

void appRadioEmotesConfused()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_CONFUSED,
    "PIPBOY_RADIO_EMOTE_CONFUSED"
  );
}
void appRadioEmotesFingerGuns()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_FINREDGU,
    "PIPBOY_RADIO_EMOTE_FINREDGU"
  );
}
void appRadioEmotesFollow()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_FOLLOW,
    "PIPBOY_RADIO_EMOTE_FOLLOW"
  );
}
void appRadioEmotesFrown()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_FROWN,
    "PIPBOY_RADIO_EMOTE_FROWN"
  );
}
void appRadioEmotesHeart()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_HEART,
    "PIPBOY_RADIO_EMOTE_HEART"
  );
}
void appRadioEmotesJoinMyTeam()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_JOINMYTE,
    "PIPBOY_RADIO_EMOTE_JOINMYTE"
  );
}
void appRadioEmotesRadiated()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_RADIATED,
    "PIPBOY_RADIO_EMOTE_RADIATED"
  );
}
void appRadioEmotesStartled()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_STARTLED,
    "PIPBOY_RADIO_EMOTE_STARTLED"
  );
}
void appRadioEmotesThumbsDown()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_THDOWN,
    "PIPBOY_RADIO_EMOTE_THDOWN"
  );
}
void appRadioEmotesThumbsUp()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_THUP,
    "PIPBOY_RADIO_EMOTE_THUP"
  );
}
void appRadioEmotesWave()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_WAVE,
    "PIPBOY_RADIO_EMOTE_WAVE"
  );
}
void appRadioEmotesFear()
{
  radioShowPreSelectedEmote(
    PIPBOY_RADIO_EMOTE_FEAR,
    "PIPBOY_RADIO_EMOTE_FEAR"
  );
}
