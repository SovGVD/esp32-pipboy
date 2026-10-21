#include "Pipboy.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

FakeDisplay display;
FakeDisplay* gfx=&display;
FakeSerial Serial;
int ESP32Encoder::useInternalWeakPullResistors=0;
ESP32Encoder menuEncoder,subMenuEncoder;
EncoderButton subMenuEncoderButton;
menuItem menuItems[MENU_MAIN_ITEMS]={};
uint8_t mainMenuNum=2,subMenuNum=0,subMenuItems=0;
uint16_t listMenuNum=0,listMenuItems=0;
bool isDisplayMessageWindow=false,isInSetupMode=false,isSubMenuEncoderControlList=false,isSubMenuButtonClicked=false,isSubMenuButtonLongPressed=false;
uint32_t hostClock=0;
uint32_t millis(){return hostClock;}
const GFXfont uiFont={nullptr,nullptr,32,126,26};
void setUiFont(){gfx->setFont(&uiFont);}
void setupSubMenu(menuItem* menu,uint8_t){menu[0].g.x=60;}
void drawMenu(menuItem*,uint8_t,uint8_t,uint16_t,uint16_t,uint16_t){}
bool isTouchTapped=false,isTouchDown=false;
int16_t touchX=-1,touchY=-1,touchDownX=-1,touchDownY=-1;
struct {int16_t x,top,width,rowHeight;uint16_t first,rows;} launcherRows={};
void touchRegisterList(int16_t x,int16_t top,int16_t width,int16_t rowHeight,uint16_t first,uint16_t rows){launcherRows={x,top,width,rowHeight,first,rows};}
bool radioChatCapturesInput(){return false;}
void radioChatTurn(int32_t){}
bool shows(const char* text){return std::find(display.texts.begin(),display.texts.end(),text)!=display.texts.end();}
void frame(){display.clear();hostClock+=32;appData();resetSubMenuButton();assert(gfx->font==&uiFont);}
void click(){onSubMenuClicked(subMenuEncoderButton);frame();}
int playerX(){
    // The renderer's player triangle has its point at y=214 and base at y=231.
    for(const auto& t:display.triangles)if(t.y1==214 && t.y2==231 && t.y3==231)return t.x1;
    return -1;
}
int main(){
    encoderSetup();frame();
    assert(listMenuItems==2 && !dataGameCapturesInput());
    click();assert(isSubMenuEncoderControlList && !dataGameCapturesInput());
    subMenuEncoder.count=2;encoderUpdate();frame();assert(listMenuNum==1);
    click();assert(dataGameCapturesInput() && shows("STAR PATROL") && shows("CLICK TO START"));
    subMenuEncoder.count+=8;encoderUpdate();frame();assert(playerX()==280); // four detents, not one
    click();assert(!shows("CLICK TO START") && !shows("PAUSED"));
    click();assert(shows("PAUSED"));
    const int x=playerX();subMenuEncoder.count+=12;encoderUpdate();frame();assert(playerX()==x);
    click();assert(!shows("PAUSED"));
    menuEncoder.count+=2;encoderUpdate();assert(mainMenuNum==3 && !dataGameCapturesInput());
    hostClock+=60000;
    menuEncoder.count-=2;encoderUpdate();assert(mainMenuNum==2 && dataGameCapturesInput());
    frame();assert(shows("PAUSED"));
    click();assert(!shows("PAUSED"));
    isDisplayMessageWindow=true;frame();assert(shows("PAUSED") && !dataGameCapturesInput());
    onSubMenuClicked(subMenuEncoderButton);assert(isSubMenuButtonClicked);
    isDisplayMessageWindow=false;resetSubMenuButton();frame();assert(shows("PAUSED"));
    onSubMenuLongPress(subMenuEncoderButton);frame();
    assert(!dataGameCapturesInput() && listMenuItems==2 && listMenuNum==1 && isSubMenuEncoderControlList);
    subMenuEncoder.count-=2;encoderUpdate();frame();assert(listMenuNum==0);
    click();assert(shows("ATOM BREAKER") && shows("CLICK TO LAUNCH"));
    dataGameTurn(std::numeric_limits<int32_t>::max());frame();
    dataGameTurn(std::numeric_limits<int32_t>::min());frame(); // clamps without overflow
    onSubMenuLongPress(subMenuEncoderButton);frame();assert(listMenuItems==2);
    onSubMenuLongPress(subMenuEncoderButton);frame();assert(!isSubMenuEncoderControlList);
    // Touch: launcher rows are registered as drawn, a tap above the ship pauses,
    // and holding on the ship row steers it toward the finger.
    assert(launcherRows.x==8 && launcherRows.top==73 && launcherRows.rowHeight==34 && launcherRows.rows==2);
    listMenuNum=1;isSubMenuEncoderControlList=true;frame();click();
    assert(dataGameCapturesInput() && shows("STAR PATROL"));
    click();assert(!shows("CLICK TO START"));
    isTouchTapped=true;touchX=240;touchY=120;dataGameTouch();isTouchTapped=false;frame();assert(shows("PAUSED"));
    isTouchTapped=true;touchY=230;dataGameTouch();isTouchTapped=false;frame();assert(shows("PAUSED")); // steering row: no click
    isTouchTapped=true;touchY=120;dataGameTouch();isTouchTapped=false;frame();assert(!shows("PAUSED"));
    const int start=playerX();
    isTouchDown=true;touchDownX=static_cast<int16_t>(start+100);touchDownY=225;
    for(int i=0;i<3;++i){dataGameTouch();frame();}
    assert(playerX()==start+90); // three frames at the 30 px/frame touch limit
    for(int i=0;i<10;++i){dataGameTouch();frame();}
    assert(playerX()==start+100); // settles on the finger
    isTouchDown=false;
    menuEncoder.count+=2;encoderUpdate();isTouchTapped=true;touchY=120;dataGameTouch();isTouchTapped=false;
    menuEncoder.count-=2;encoderUpdate();frame();assert(shows("PAUSED")); // ignored while DATA is hidden
    std::cout<<"Arcade controls: full detents, launcher, pause, tab return, modal dismissal, back, touch steering/pause and font restoration passed\n";
}
