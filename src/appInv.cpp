#include "Pipboy.h"
#include <Inventory.h>
#include "assets/fallout10.h"

namespace {
bool menuReady = false;
menuItem inventoryMenu[] = {
  {"WEAPONS", 7, {}, appInvWeapons},
  {"APPAREL", 7, {}, appInvApparel},
  {"AID",     3, {}, appInvAid},
  {"MISC",    4, {}, appInvMisc},
  {"JUNK",    4, {}, appInvJunk},
};

class DisplayCanvas : public inventory::Canvas {
public:
  void line(int x1, int y1, int x2, int y2, uint16_t color) override {
    gfx->drawLine(x1, y1, x2, y2, color);
  }
  void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) override {
    gfx->fillTriangle(x1, y1, x2, y2, x3, y3, color);
  }
  void rectangle(int x, int y, int width, int height, uint16_t color) override {
    gfx->fillRect(x, y, width, height, color);
  }
  void text(int x, int baseline, const char* value, uint16_t color) override {
    gfx->setTextColor(color);
    gfx->setCursor(x, baseline);
    gfx->print(value);
  }
};

void drawCategory(size_t index) {
  const auto* category = inventory::category(index);
  listMenuItems = category ? category->count : 0;
  listMenuNum = category ? inventory::clampSelection(*category, listMenuNum) : 0;
  DisplayCanvas canvas;
  gfx->setFont(&monofonto10pt7b);
  inventory::render(canvas, index, listMenuNum, isSubMenuEncoderControlList);
  if (category) {
    const unsigned first = inventory::firstVisible(*category, listMenuNum);
    touchRegisterList(inventory::listX, inventory::listTop, inventory::listWidth, inventory::listRowHeight,
                      first, min<unsigned>(inventory::visibleRows, category->count - first));
  }
  setUiFont();
}
}

void setupInvMenu() {
  setupSubMenu(inventoryMenu, inventory::categoryCount);
  menuReady = true;
}

void appInv() {
  if (!menuReady) setupInvMenu();
  subMenuItems = inventory::categoryCount;
  if (subMenuNum >= subMenuItems) subMenuNum = 0;
  drawMenu(inventoryMenu, subMenuItems, subMenuNum, COLOR_MAIN, COLOR_INACTIVE,
           menuItems[1].g.x - inventoryMenu[subMenuNum].g.x);
  inventoryMenu[subMenuNum].app();
}

void appInvWeapons() { drawCategory(0); }
void appInvApparel() { drawCategory(1); }
void appInvAid()     { drawCategory(2); }
void appInvMisc()    { drawCategory(3); }
void appInvJunk()    { drawCategory(4); }
