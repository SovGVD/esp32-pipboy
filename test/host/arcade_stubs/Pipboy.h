#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct GFXglyph { uint16_t bitmapOffset; uint8_t width,height,xAdvance; int8_t xOffset,yOffset; };
struct GFXfont { uint8_t* bitmap; GFXglyph* glyph; uint16_t first,last; uint8_t yAdvance; };
struct FakeDisplay {
    const GFXfont* font = nullptr;
    std::vector<std::string> texts;
    struct Triangle { int x1,y1,x2,y2,x3,y3; };
    std::vector<Triangle> triangles;
    void drawLine(int,int,int,int,uint16_t) {}
    void fillTriangle(int x1,int y1,int x2,int y2,int x3,int y3,uint16_t) {triangles.push_back({x1,y1,x2,y2,x3,y3});}
    void fillRect(int,int,int,int,uint16_t) {}
    void setCursor(int,int) {}
    void setTextColor(uint16_t) {}
    void print(const char* value) {texts.emplace_back(value);}
    void setFont(const GFXfont* value) {font=value;}
    void clear() {texts.clear();triangles.clear();}
};
extern FakeDisplay* gfx;
struct itemGeometry {uint16_t x,y,w,h;};
struct menuItem {char name[127];uint8_t nameLength;itemGeometry g;void(*app)();};
#define MENU_MAIN_ITEMS 5
#define COLOR_MAIN 0x45a2
#define COLOR_INACTIVE 0x2201
#define MENU_ENCODER_PIN_A 16
#define MENU_ENCODER_PIN_B 15
#define SUB_MENU_ENCODER_PIN_A 17
#define SUB_MENU_ENCODER_PIN_B 18
namespace puType { constexpr int up=1; }
struct ESP32Encoder {
    static int useInternalWeakPullResistors;
    int64_t count=0;
    int64_t getCount() {return count;}
    void attachHalfQuad(int,int) {}
};
struct EncoderButton {
    void update() {}
    void setClickHandler(void(*)(EncoderButton&)) {}
    void setLongPressHandler(void(*)(EncoderButton&),bool) {}
};
struct FakeSerial {void println(const char*) {}};
extern FakeSerial Serial;
extern ESP32Encoder menuEncoder,subMenuEncoder;
extern EncoderButton subMenuEncoderButton;
extern menuItem menuItems[MENU_MAIN_ITEMS];
extern uint8_t mainMenuNum,subMenuNum,subMenuItems;
extern uint16_t listMenuNum,listMenuItems;
extern bool isDisplayMessageWindow,isInSetupMode,isSubMenuEncoderControlList,isSubMenuButtonClicked,isSubMenuButtonLongPressed;
uint32_t millis();
void setupSubMenu(menuItem*,uint8_t);
void drawMenu(menuItem*,uint8_t,uint8_t,uint16_t,uint16_t,uint16_t);
void setUiFont();
void appData();
bool dataGameCapturesInput();
void dataGameTurn(int32_t);
void dataGameClick();
void dataGameBack();
void dataGameVisibility();
bool radioChatCapturesInput();
void radioChatTurn(int32_t);
void onSubMenuClicked(EncoderButton&);
void onSubMenuLongPress(EncoderButton&);
void updateMenuByEncoder();
void updateSubMenuByEncoder();
void updateListByEncoder();
void resetSubMenu();
void resetListItemMenu();
void resetSubMenuButton();
void encoderSetup();
void encoderUpdate();
