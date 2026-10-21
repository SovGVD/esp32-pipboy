#include <Touch.h>
#include "Pipboy.h"

/**
 * Turns taps into the same state changes the encoders make. Screens register
 * what they drew this frame; the next frame's tap is matched against that, so
 * it always hits what the user saw.
 *
 * - Main tab: switch to it.
 * - Subpage label: switch to it; tapping the current one goes back (like hold).
 * - List row: select it; tapping the selected row activates it (like click).
 */
namespace {
const int16_t TOUCH_MAIN_MENU_BOTTOM = 28;
const int16_t TOUCH_SUB_MENU_BOTTOM = 58;

menuItem* touchSubMenu = nullptr;
uint8_t touchSubMenuItems = 0;
int16_t touchSubMenuDelta = 0;
bool isTouchListRegistered = false;
touch::ListRegion touchList = {};

bool touchMainMenu()
{
  uint8_t nearest = 0;
  int16_t nearestDistance = INT16_MAX;
  for (uint8_t idx = 0; idx < MENU_MAIN_ITEMS; idx++) {
    const int16_t center = menuItems[idx].g.x + menuItems[idx].g.w / 2;
    const int16_t distance = abs(touchX - center);
    if (distance < nearestDistance) {
      nearest = idx;
      nearestDistance = distance;
    }
  }

  if (nearest != mainMenuNum) {
    mainMenuNum = nearest;
    resetSubMenu();
    resetListItemMenu();
  }
  return true;
}

bool touchSubMenuLabel()
{
  for (uint8_t idx = 0; touchSubMenu && idx < touchSubMenuItems; idx++) {
    const int16_t left = touchSubMenu[idx].g.x + touchSubMenuDelta - MENU_ITEMS_PADDING_SUBMENU / 2;
    const int16_t right = left + touchSubMenu[idx].g.w + MENU_ITEMS_PADDING_SUBMENU;
    if (touchX < left || touchX >= right) {
      continue;
    }

    if (idx == subMenuNum) {
      onSubMenuLongPress(subMenuEncoderButton);
    } else {
      subMenuNum = idx;
      resetListItemMenu();
    }
    return true;
  }

  return false;
}
}

void touchRegisterSubMenu(menuItem* menu, uint8_t menuItemsNum, int16_t menuXdelta)
{
  touchSubMenu = menu;
  touchSubMenuItems = menuItemsNum;
  touchSubMenuDelta = menuXdelta;
}

void touchRegisterList(int16_t x, int16_t top, int16_t width, int16_t rowHeight, uint16_t first, uint16_t rows)
{
  touchList = {x, top, width, rowHeight, first, rows};
  isTouchListRegistered = true;
}

bool touchListTap()
{
  if (!isTouchListRegistered) {
    return false;
  }

  const int index = touch::listIndexAt(touchList, touchX, touchY);
  if (index < 0) {
    return false;
  }

  if (isSubMenuEncoderControlList && index == listMenuNum) {
    isSubMenuButtonClicked = true;
  } else {
    listMenuNum = index;
    isSubMenuEncoderControlList = true;
  }
  return true;
}

void touchClearRegions()
{
  touchSubMenu = nullptr;
  touchSubMenuItems = 0;
  isTouchListRegistered = false;
}

void touchDispatch()
{
  if (isTouchTapped && !isDisplayMessageWindow) {
    if (touchY < TOUCH_MAIN_MENU_BOTTOM) {
      touchMainMenu();
    } else if (touchY < TOUCH_SUB_MENU_BOTTOM) {
      touchSubMenuLabel();
    } else if (!dataGameCapturesInput()) {
      touchListTap();
    }
  }

  // A held finger steers a running game; taps there are handled by the game too.
  dataGameTouch();
  touchClearRegions();
}
