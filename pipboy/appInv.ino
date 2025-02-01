#define APP_INV_MENU_ITEMS 5
bool isInvMenuReady = false;

// STATUS SPECIAL PERKS
menuItem appInvMenu[APP_INV_MENU_ITEMS] = {
  {"WEAPONS", 7, {0, 0, 0, 0}, appInvWeapons},
  {"APPAREL", 7, {0, 0, 0, 0}, appInvApparel},
  {"AID",     3, {0, 0, 0, 0}, appInvAid},
  {"MISC",    4, {0, 0, 0, 0}, appInvMisc},
  {"JUNK",    4, {0, 0, 0, 0}, appInvJunk},
};

void setupInvMenu()
{
  setupSubMenu(appInvMenu, APP_INV_MENU_ITEMS);
}

void appInv()
{
  if (!isInvMenuReady) {
    setupInvMenu();
  }

  subMenuItems = APP_INV_MENU_ITEMS;

  drawMenu(
    appInvMenu, 
    APP_INV_MENU_ITEMS, 
    subMenuNum,
    COLOR_MAIN,
    COLOR_INACTIVE,
    menuItems[1].g.x - appInvMenu[subMenuNum].g.x
  );

  appInvMenu[subMenuNum].app();
}

void appInvWeapons() {

}
void appInvApparel() {

}
void appInvAid() {

}
void appInvMisc() {

}
void appInvJunk() {

}
