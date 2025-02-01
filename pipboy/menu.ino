uint16_t hl_bottomLine;

void setupMenu()
{
  for(uint8_t i = 0; i < MENU_MAIN_ITEMS; i++) {
    menuItems[i].g.w = menuItems[i].nameLength * CHAR_WIDTH;
    menuGeometry.w = menuGeometry.w + menuItems[i].g.w + (i!=0 ? MENU_ITEMS_PADDING : 0);
  }
  menuGeometry.x = (DISPLAY_WIDTH - menuGeometry.w)/2;
  menuGeometry.y = MENU_PADDING_TOP;
  menuGeometry.h = CHAR_HEIGHT;

  hl_bottomLine = menuGeometry.y + menuGeometry.h + MENU_HIGHLIGHT_BOTTOM_LINE;

  subMenuGeometry.x = menuGeometry.x;
  subMenuGeometry.y = hl_bottomLine + MENU_HIGHLIGHT_PADDING;
  subMenuGeometry.h = menuGeometry.h;

  updateMenuGeometry(menuItems, MENU_MAIN_ITEMS, MENU_ITEMS_PADDING, menuGeometry);
}

void setupSubMenu(menuItem* menu, uint8_t menuItemsNum)
{
  updateMenuGeometry(menu, menuItemsNum, MENU_ITEMS_PADDING_SUBMENU, subMenuGeometry);
}

menuItem getCurrentMenuItem()
{
  return menuItems[mainMenuNum];
}

void drawMainMenu()
{
  drawMenu(menuItems, MENU_MAIN_ITEMS, mainMenuNum, COLOR_MAIN, COLOR_MAIN, 0);
  drawMainMenuHighlights(COLOR_MAIN);
}

void drawMainMenuHighlights(uint16_t color)
{
  uint16_t pos = menuItems[mainMenuNum].g.x;
  uint16_t length = menuItems[mainMenuNum].g.w;
  uint16_t middle = menuItems[mainMenuNum].g.y + menuItems[mainMenuNum].g.h / 2;

  // gfx->drawRect(menuItems[mainMenuNum].g.x, menuItems[mainMenuNum].g.y, menuItems[mainMenuNum].g.w, menuItems[mainMenuNum].g.h, COLOR_DEBUG);

  for (uint8_t i = 0; i < MENU_LINE_WIDTH; i++) {
    gfx->drawLine(
      0, hl_bottomLine + i, 
      pos - MENU_HIGHLIGHT_LINE - MENU_HIGHLIGHT_PADDING, hl_bottomLine + i,
      color);

    gfx->drawLine(
      pos - MENU_HIGHLIGHT_LINE - MENU_HIGHLIGHT_PADDING - i, hl_bottomLine ,
      pos - MENU_HIGHLIGHT_LINE - MENU_HIGHLIGHT_PADDING - i, middle,
      color);

    gfx->drawLine(
      pos - MENU_HIGHLIGHT_LINE - MENU_HIGHLIGHT_PADDING, middle + i,
      pos - MENU_HIGHLIGHT_PADDING, middle + i,
      color);

    gfx->drawLine(
      pos + length + MENU_HIGHLIGHT_PADDING , middle + i,
      pos + length + MENU_HIGHLIGHT_LINE + MENU_HIGHLIGHT_PADDING, middle + i,
      color);

    gfx->drawLine(
      pos + length + MENU_HIGHLIGHT_LINE + MENU_HIGHLIGHT_PADDING + i, middle,
      pos + length + MENU_HIGHLIGHT_LINE + MENU_HIGHLIGHT_PADDING + i, hl_bottomLine,
      color);

    gfx->drawLine(
      pos + length + MENU_HIGHLIGHT_LINE + MENU_HIGHLIGHT_PADDING, hl_bottomLine + i,
      DISPLAY_WIDTH, hl_bottomLine + i,
      color);
  }
}
