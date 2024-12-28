#define APP_STAT_MENU_ITEMS 3
bool isStatMenuReady = false;

#define APP_STAT_SPECIAL_X 200
#define APP_STAT_SPECIAL_Y 87
#define APP_STAT_SPECIAL_W 20
bool isStatSpecialReady = false;

// STATUS SPECIAL PERKS
menuItem appStatMenu[APP_STAT_MENU_ITEMS] = {
  {"STATUS",  6, {0, 0, 0, 0}, appStatus},
  {"SPECIAL", 7, {0, 0, 0, 0}, appSpecial},
  {"PERKS",   5, {0, 0, 0, 0}, appPerks},
};

#define APP_STAT_SPECIAL_MENU_ITEMS 7
menuItem appStatSpecialMenu[APP_STAT_SPECIAL_MENU_ITEMS] = {
  {"Strength      00", 16, {0, 0, 0, 0}, appStatSpecialStrength},
  {"Perception    00", 16, {0, 0, 0, 0}, appStatSpecialPerception},
  {"Endurance     00", 16, {0, 0, 0, 0}, appStatSpecialEndurance},
  {"Charisma      00", 16, {0, 0, 0, 0}, appStatSpecialCharisma},
  {"Intelligence  00", 16, {0, 0, 0, 0}, appStatSpecialIntelligence},
  {"Agility       00", 16, {0, 0, 0, 0}, appStatSpecialAgility},
  {"Luck          00", 16, {0, 0, 0, 0}, appStatSpecialLuck},
};

void setupStat()
{
    gif.open((uint8_t *)vaultboy, sizeof(vaultboy), GIFDraw);
}

void setupStatMenu()
{
  setupSubMenu(appStatMenu, APP_STAT_MENU_ITEMS);
}

void appStat()
{
  if (!isStatMenuReady) {
    setupStatMenu();
  }

  subMenuItems = APP_STAT_MENU_ITEMS;

  drawMenu(
    appStatMenu, 
    APP_STAT_MENU_ITEMS, 
    subMenuNum,
    COLOR_MAIN,
    COLOR_INACTIVE,
    menuItems[0].g.x - appStatMenu[subMenuNum].g.x
  );

  appStatMenu[subMenuNum].app();
  appStatBar();
}

void appStatus()
{
  gif.playFrame(true, NULL);
}

void appStatBar()
{
  gfx->fillRect(0, h - STATUS_BAR_HEIGHT, STATUS_BAR_SIDE_WIDTH, STATUS_BAR_HEIGHT, COLOR_BACKGROUND);

  gfx->setCursor(5, h - STATUS_BAR_HEIGHT + FONT_PADDING);
  gfx->setTextColor(COLOR_MAIN);
  gfx->printf("HP %d/100", stat_hp);

  gfx->fillRect(STATUS_BAR_SIDE_WIDTH + STATUS_BAR_PADDING, h - STATUS_BAR_HEIGHT, w - STATUS_BAR_SIDE_WIDTH*2 - STATUS_BAR_PADDING * 2, STATUS_BAR_HEIGHT, COLOR_BACKGROUND);

  gfx->setCursor(STATUS_BAR_SIDE_WIDTH + STATUS_BAR_PADDING + 5, h - STATUS_BAR_HEIGHT + FONT_PADDING);
  gfx->setTextColor(COLOR_MAIN);
  gfx->printf("LEVEL %d", stat_level);

  gfx->drawRect(
    STATUS_BAR_SIDE_WIDTH + STATUS_BAR_PADDING + LEVEL_TEXT_LENGTH, 
    h - STATUS_BAR_HEIGHT + 10, 
    w - STATUS_BAR_SIDE_WIDTH*2 - STATUS_BAR_PADDING * 2 - LEVEL_TEXT_LENGTH - 10, 
    STATUS_BAR_HEIGHT - 20, 
    COLOR_MAIN);

  gfx->fillRect(
    STATUS_BAR_SIDE_WIDTH + STATUS_BAR_PADDING + LEVEL_TEXT_LENGTH + 2, 
    h - STATUS_BAR_HEIGHT + 10 + 2, 
    w - STATUS_BAR_SIDE_WIDTH*2 - STATUS_BAR_PADDING * 2 - LEVEL_TEXT_LENGTH - 10 - stat_progress, 
    STATUS_BAR_HEIGHT - 20 - 4, 
    COLOR_MAIN);


  gfx->fillRect(w - STATUS_BAR_SIDE_WIDTH, h - STATUS_BAR_HEIGHT, STATUS_BAR_SIDE_WIDTH, STATUS_BAR_HEIGHT, COLOR_BACKGROUND);

  gfx->setCursor(w - STATUS_BAR_SIDE_WIDTH + 24, h - STATUS_BAR_HEIGHT + FONT_PADDING);
  gfx->setTextColor(COLOR_MAIN);
  gfx->printf("AP %d/70", stat_ap);
}

/**
 * Stat -> SPECIAL
 */
void setupStatSpecial()
{
  for(uint8_t i = 0; i < APP_STAT_SPECIAL_MENU_ITEMS; i++) {
    uint8_t stat = random(0, 13);
    if (stat < 1) stat = 1;
    if (stat > 10) stat = 10;
    if (stat == 10) {
      appStatSpecialMenu[i].name[14] = 49;
      appStatSpecialMenu[i].name[15] = 48;
    } else {
      appStatSpecialMenu[i].name[14] = 32;
      appStatSpecialMenu[i].name[15] = stat + 48;
    }
  }
  isStatSpecialReady = true;
}

void appSpecial()
{
  if (!isStatSpecialReady) {
    setupStatSpecial();
  }

  listMenuItems = APP_STAT_SPECIAL_MENU_ITEMS;

  drawListMenu(
    appStatSpecialMenu,
    APP_STAT_SPECIAL_MENU_ITEMS,
    listMenuNum,
    COLOR_MAIN,
    COLOR_SCREEN,
    20,
    APP_STAT_SPECIAL_Y,
    16
  );
  appStatSpecialMenu[listMenuNum].app();
}

void appStatSpecialStrength()
{
  displayTxt("Strength is a measure of your raw physical power. It affects how much you can carry, and the damage of all melee attacks.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appStatSpecialPerception()
{
  displayTxt("Perception is your     environmental awareness and \"sixth sense\", and affects weapon accuracy in VATS.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appStatSpecialEndurance()
{
  displayTxt("Endurance is a measure of your overall physical fitness. It affects your total Health and the Action Point drain from sprinting.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appStatSpecialCharisma()
{
  displayTxt("Charisma is your ability to charm and convince others. It affects your success to persuade in dialogue and prices when you barter.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appStatSpecialIntelligence()
{
  displayTxt("Intelligence is a measure of your overall mental acuity, and affects the number of Experience Points earned.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appStatSpecialAgility()
{
  displayTxt("Agility is a measure of your overall finesse and reflexes. It affects the number of Action Points in VATS and your ability to sneak.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appStatSpecialLuck()
{
  displayTxt("Luck is a measure of your general good fortune, and affects the recharge rate of Critical Hits.", 
  APP_STAT_SPECIAL_X, APP_STAT_SPECIAL_Y, APP_STAT_SPECIAL_W);
}

void appPerks()
{

}
