#include <WiFi.h>
#include <QuickEspNow.h>
#include "MusicPlayer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Chat.h>
#include "Pipboy.h"
#include "assets/fallout10.h"
const char* PIPBOY_RADIO_DELIMETER = "\t";  // tab
const char* PIPBOY_RADIO_HEADER = "PIPBOY";
const char* PIPBOY_RADIO_PING = "P";
const char* PIPBOY_RADIO_MESSAGE = "M"; // Text chat message: "<4 hex id> <text>"
const char* PIPBOY_RADIO_ACK = "A"; // Direct message received: "<4 hex id>"
const char* PIPBOY_RADIO_EMOTE = "E"; // Emote, only received from older firmware

const char* PIPBOY_RADIO_EMPTY = "_";

struct radioLegacyEmote {
  const char* code;
  const char* text;
};

// @src https://fallout.fandom.com/wiki/Atomic_Shop/Emotes
const radioLegacyEmote PIPBOY_RADIO_LEGACY_EMOTES[] = {
  {"confused", "Confused"},
  {"fingergu", "Finger Guns"},
  {"follow",   "Follow!"},
  {"frown",    "Frown"},
  {"heart",    "Heart"},
  {"joinmyte", "Join My Team!"},
  {"radiated", "Radiated"},
  {"startled", "Startled"},
  {"thdown",   "Thumbs Down"},
  {"thup",     "Thumbs Up"},
  {"wave",     "Wave"},
  {"fear",     "Fear"},
};

uint8_t pipboyPeersCount = 0;
unsigned long offlineAfter;
const unsigned long pipboyPeerTTL = 300; // 300 ticks is about 10 seconds (1 tick per frame), survives two lost pings
const unsigned long PIPBOY_RADIO_PING_TTL = 3000000; // microseconds (10^-6 seconds)

unsigned long previousPingTime;

#define APP_RADIO_PEERS_Y 87
#define PIPBOY_RADIO_UNKNOWN_PEER 255
#define APP_RADIO_MAIN_MENU_ITEM 4
#define APP_RADIO_CHAT_MENU_ITEM 1
#define APP_RADIO_POPUP_LINE_WIDTH 34

#define APP_RADIO_MENU_ITEMS 2
bool isRadioMenuReady = false;

menuItem appRadioMenu[APP_RADIO_MENU_ITEMS] = {
  {"MUSIC", 5, {0, 0, 0, 0}, appPlayer},
  {"CHAT",  4, {0, 0, 0, 0}, appChat},
};

namespace {
chat::History chatHistory;
chat::RecentIds chatReceivedIds;
uint16_t chatNextId = 0;
chat::Composer chatComposer;
bool isChatOpen = false;
bool isChatBroadcast = false;
char chatPeer[sizeof(radioPeer::name)] = {};

class ChatCanvas : public chat::Canvas {
public:
  void line(int x1, int y1, int x2, int y2, uint16_t color) override { gfx->drawLine(x1, y1, x2, y2, color); }
  void rectangle(int x, int y, int width, int height, uint16_t color) override { gfx->fillRect(x, y, width, height, color); }
  void text(int x, int baseline, const char* text, uint16_t color) override {
    gfx->setCursor(x, baseline);
    gfx->setTextColor(color);
    gfx->print(text);
  }
};
}

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
    menuItems[APP_RADIO_MAIN_MENU_ITEM].g.x - appRadioMenu[subMenuNum].g.x
  );

  appRadioMenu[subMenuNum].app();
}

/**
 * /!\ Please don't expect any security/privacy/crtypto features, that is as simple as possible /!\
 */
void appChat()
{
  radioUpdatePipboyPeersCount();

  if (isChatOpen) {
    if (!isSubMenuButtonLongPressed || isDisplayMessageWindow) {
      appRadioChatWindow();
      return;
    }
    // Hold goes back one level: from the conversation to the peer list.
    isChatOpen = false;
    isSubMenuEncoderControlList = true;
    listMenuNum = 0;
  }

  if (pipboyPeersCount == 0) {
    appRadioNoSignal();
    return;
  }

  appRadioPeerSelector();
}

void appRadioChatWindow()
{
  bool isKeyPressed = false;
  if (isTouchTapped && !isDisplayMessageWindow) {
    const unsigned key = chat::keyIndexAt(touchX, touchY);
    if (key < chat::keyCount) {
      // A tap types immediately, even while the encoder is on subpage selection.
      chatComposer.select(key);
      isSubMenuEncoderControlList = true;
      isKeyPressed = true;
    }
  }

  const bool isTyping = isSubMenuEncoderControlList && !isDisplayMessageWindow;
  if (isTyping && (isSubMenuButtonClicked || isKeyPressed)) {
    char text[chat::maxText + 1];
    if (chatComposer.press(text)) {
      radioSendChat(text);
    }
  }

  ChatCanvas canvas;
  const chat::View view = {
    &chatHistory,
    &chatComposer,
    chatPeer,
    isChatBroadcast,
    radioIsPeerOnline(chatPeer),
    isTyping
  };
  gfx->setFont(&monofonto10pt7b);
  chat::render(canvas, view);
  setUiFont();
}

bool radioChatCapturesInput()
{
  return mainMenuNum == APP_RADIO_MAIN_MENU_ITEM && subMenuNum == APP_RADIO_CHAT_MENU_ITEM &&
    isChatOpen && isSubMenuEncoderControlList && !isDisplayMessageWindow && !isInSetupMode;
}

void radioChatTurn(int32_t turn)
{
  chatComposer.turn(turn);
}

void appRadioPeerSelector()
{
  // First entry talks to everyone nearby, the rest are online peers.
  menuItem peers[PIPBOY_RADIO_MAX_PEERS + 1] = {};
  uint8_t peerIndexes[PIPBOY_RADIO_MAX_PEERS + 1] = {};
  strcpy(peers[0].name, "ALL");
  peers[0].nameLength = 3;
  uint8_t menuIdx = 1;

  for (uint8_t idx = 0; idx < PIPBOY_RADIO_MAX_PEERS; idx++) {
    if (!pipboyPeers[idx].isOnline) {
      continue;
    }

    strncpy(peers[menuIdx].name, pipboyPeers[idx].name, pipboyPeers[idx].nameLength);
    peers[menuIdx].name[pipboyPeers[idx].nameLength] = '\0';
    peers[menuIdx].nameLength = pipboyPeers[idx].nameLength;
    peerIndexes[menuIdx] = idx;

    menuIdx++;
  }

  listMenuItems = menuIdx;
  if (listMenuNum >= listMenuItems) listMenuNum = listMenuItems - 1;

  drawListMenu(
    peers,
    menuIdx,
    listMenuNum,
    COLOR_MAIN,
    COLOR_SCREEN,
    20,
    APP_RADIO_PEERS_Y,
    16
  );

  if (!isSubMenuButtonClicked || isDisplayMessageWindow) {
    return;
  }

  isChatBroadcast = listMenuNum == 0;
  strcpy(chatPeer, isChatBroadcast ? PIPBOY_RADIO_EMPTY : pipboyPeers[peerIndexes[listMenuNum]].name);
  isChatOpen = true;
}

void appRadioNoSignal()
{
  gfx->setCursor(20, APP_RADIO_PEERS_Y);
  gfx->setTextColor(COLOR_MAIN);
  gfx->print("NO SIGNAL");
}

bool radioIsPeerOnline(const char* peerName)
{
  for (uint8_t idx = 0; idx < PIPBOY_RADIO_MAX_PEERS; idx++) {
    if (pipboyPeers[idx].isOnline && strcmp(pipboyPeers[idx].name, peerName) == 0) {
      return true;
    }
  }

  return false;
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
  // A random start keeps ids fresh after a reboot, so peers do not drop them as duplicates.
  chatNextId = random(0, 0x10000);
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


bool radioIsPrintable(const char* text)
{
  for (const char* c = text; *c; c++) {
    if (*c < 0x20 || *c > 0x7e) return false;
  }

  return true;
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

  if (strcmp(cmd, PIPBOY_RADIO_MESSAGE) == 0) {
    processRadioMessageText(from, body, strcmp(to, PIPBOY_RADIO_EMPTY) == 0);
    return;
  }

  if (strcmp(cmd, PIPBOY_RADIO_ACK) == 0) {
    uint16_t id = 0;
    if (chat::parseId(body, id)) {
      chatHistory.acknowledge(from, id);
    }
    return;
  }

  if (strcmp(cmd, PIPBOY_RADIO_EMOTE) == 0) {
    processRadioMessageEmote(from, body, strcmp(to, PIPBOY_RADIO_EMPTY) == 0);
    return;
  }
}

void processRadioMessageText(char* from, char* body, bool isBroadcast)
{
  uint16_t id = 0;
  const char* text = nullptr;
  if (!chat::parseBody(body, id, text)) {
    return;
  }

  if (!isBroadcast) {
    // Acknowledge every copy: a resend means our previous ACK was lost.
    char ack[8];
    chat::formatId(ack, sizeof(ack), id);
    radioSend(radioMessage(PIPBOY_RADIO_ACK, from, ack, false));
  }

  if (chatReceivedIds.seen(from, id)) {
    return;
  }

  processRadioMessageChat(from, text, isBroadcast);
}

void processRadioMessageChat(const char* from, const char* text, bool isBroadcast)
{
  char cleanText[chat::maxText + 1];
  if (!chat::sanitize(cleanText, sizeof(cleanText), text)) {
    return;
  }

  const chat::Message& message = chatHistory.add(from, cleanText, false, isBroadcast);

  // No popup for the conversation that is already on screen.
  const bool isVisible = mainMenuNum == APP_RADIO_MAIN_MENU_ITEM && subMenuNum == APP_RADIO_CHAT_MENU_ITEM &&
    isChatOpen && chat::inConversation(message, chatPeer, isChatBroadcast);
  if (isVisible) {
    return;
  }

  chat::Line lines[4];
  const size_t linesCount = min(chat::wrap(message.text, APP_RADIO_POPUP_LINE_WIDTH, lines, 4), (size_t)4);
  String body;
  for (size_t idx = 0; idx < linesCount; idx++) {
    if (idx) body += "\n";
    body += String(message.text + lines[idx].start).substring(0, lines[idx].length);
  }
  setDisplayMessage(isBroadcast ? "Message to all" : "Incoming message", from, body.c_str());
}

void processRadioMessageEmote(char* from, char* emote, bool isBroadcast)
{
  for (const radioLegacyEmote& legacyEmote : PIPBOY_RADIO_LEGACY_EMOTES) {
    if (strcmp(emote, legacyEmote.code) == 0) {
      const String text = "*" + String(legacyEmote.text) + "*";
      processRadioMessageChat(from, text.c_str(), isBroadcast);
      return;
    }
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
        strlen(to) >= sizeof(radioPeer::name) || !radioIsPrintable(from)) continue;
    processRadioMessage(cmd, from, to, body, packet.rssi);
  }
  radioRetryChat();
  radioPing();
}

void radioSendChat(const char* text)
{
  const chat::Message& message = chatHistory.add(chatPeer, text, true, isChatBroadcast, chatNextId++, millis());
  radioSendChatMessage(message);
}

void radioSendChatMessage(const chat::Message& message)
{
  char body[chat::maxText + 8];
  if (!chat::formatBody(body, sizeof(body), message.id, message.text)) {
    return;
  }

  radioSend(radioMessage(PIPBOY_RADIO_MESSAGE, (char*)message.peer, body, message.broadcast));
}

void radioRetryChat()
{
  // Direct messages are resent until acknowledged or out of attempts.
  while (chat::Message* message = chatHistory.dueForRetry(millis())) {
    radioSendChatMessage(*message);
  }
}
