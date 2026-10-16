#include "Pipboy.h"
#include "assets/fallout10.h"
#include <Arcade.h>
#include <algorithm>

namespace {
arcade::Session sessions[] = {arcade::Session(arcade::Game::Breaker),arcade::Session(arcade::Game::Patrol)};
int openGame = -1;
int pendingTurn = 0;
bool pendingClick = false;
uint32_t lastGameTime = 0;
bool menuReady = false;
menuItem dataMenu[] = {{"GAMES",5,{},nullptr}};

class GameCanvas : public arcade::Canvas {
public:
    void line(int x1,int y1,int x2,int y2,uint16_t color) override { gfx->drawLine(x1,y1,x2,y2,color); }
    void triangle(int x1,int y1,int x2,int y2,int x3,int y3,uint16_t color) override {
        gfx->fillTriangle(x1,y1,x2,y2,x3,y3,color);
    }
    void rectangle(int x,int y,int width,int height,uint16_t color) override { gfx->fillRect(x,y,width,height,color); }
    void text(int x,int baseline,const char* text,uint16_t color) override {
        gfx->setCursor(x,baseline); gfx->setTextColor(color); gfx->print(text);
    }
};
}

bool dataGameCapturesInput() {
    return mainMenuNum == 2 && openGame >= 0 && !isDisplayMessageWindow && !isInSetupMode;
}

void dataGameTurn(int32_t turn) {
    // Preserve all detents between frames. Clamp before addition to tolerate a
    // disconnected/glitching encoder without overflow or a loop proportional to it.
    pendingTurn = std::max(-100,std::min(100,pendingTurn+static_cast<int>(std::max<int32_t>(-100,std::min<int32_t>(100,turn)))));
}

void dataGameClick() { pendingClick = true; }

void dataGameBack() {
    if (openGame < 0) return;
    sessions[openGame].pause();
    listMenuNum = openGame;
    openGame = -1;
    pendingTurn = 0; pendingClick = false;
    isSubMenuEncoderControlList = true;
}

void dataGameVisibility() {
    if (openGame >= 0 && !dataGameCapturesInput()) {
        sessions[openGame].pause();
        pendingTurn = 0; pendingClick = false;
        lastGameTime = millis();
    }
}

void appData() {
    if (!menuReady) { setupSubMenu(dataMenu,1); menuReady = true; }
    subMenuNum = 0;
    subMenuItems = 1;
    drawMenu(dataMenu,1,0,COLOR_MAIN,COLOR_INACTIVE,menuItems[2].g.x-dataMenu[0].g.x);
    dataGameVisibility();
    GameCanvas canvas;
    gfx->setFont(&monofonto10pt7b);
    if (openGame < 0) {
        listMenuItems = arcade::gameCount;
        if (listMenuNum >= listMenuItems) listMenuNum = 0;
        if (isSubMenuButtonClicked && !isDisplayMessageWindow) {
            openGame = listMenuNum;
            isSubMenuEncoderControlList = true;
            pendingTurn = 0; pendingClick = false;
            lastGameTime = millis();
        }
    }
    if (openGame >= 0) {
        listMenuItems = 0;
        canvas.text(278,50,arcade::name(sessions[openGame].state().game),COLOR_MAIN);
        const uint32_t now = millis();
        if (dataGameCapturesInput()) sessions[openGame].update(now-lastGameTime,{pendingTurn,pendingClick});
        lastGameTime = now;
        pendingTurn = 0; pendingClick = false;
        arcade::render(canvas,sessions[openGame].state());
    } else arcade::renderLauncher(canvas,listMenuNum,isSubMenuEncoderControlList,sessions[listMenuNum].state().best);
    setUiFont();
}
