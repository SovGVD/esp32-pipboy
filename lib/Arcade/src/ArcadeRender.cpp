#include "Arcade.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace arcade {
namespace {
constexpr uint16_t ink = 0x45a2, dim = 0x2201, black = 0;
int pixel(float value) { return static_cast<int>(std::lround(value)); }
void box(Canvas& canvas, int x, int y, int width, int height, uint16_t color) {
    canvas.line(x,y,x+width-1,y,color); canvas.line(x,y+height-1,x+width-1,y+height-1,color);
    canvas.line(x,y,x,y+height-1,color); canvas.line(x+width-1,y,x+width-1,y+height-1,color);
}
void centered(Canvas& canvas, int baseline, const char* text) {
    canvas.text((480-std::strlen(text)*10)/2,baseline,text,ink);
}
void ship(Canvas& canvas, int x, int y) {
    canvas.triangle(x,y-10,x-10,y+7,x+10,y+7,ink);
    canvas.triangle(x,y-4,x-3,y+7,x+3,y+7,black);
    canvas.rectangle(x-12,y+2,4,8,ink); canvas.rectangle(x+9,y+2,4,8,ink);
}
void alien(Canvas& canvas, int x, int y, unsigned row, bool flap) {
    const int wing = flap ? 7 : 3;
    canvas.rectangle(x-3,y-5,7,10,ink);
    canvas.triangle(x-4,y-3,x-11,y-wing,x-9,y+6,ink);
    canvas.triangle(x+4,y-3,x+11,y-wing,x+9,y+6,ink);
    canvas.rectangle(x-1,y-2,3,3,black);
    if (!row) { canvas.line(x-4,y-6,x-7,y-9,ink); canvas.line(x+4,y-6,x+7,y-9,ink); }
    else if (row == 1) { canvas.line(x-6,y+5,x-6,y+9,ink); canvas.line(x+6,y+5,x+6,y+9,ink); }
}
}

const char* name(Game game) { return game == Game::Breaker ? "ATOM BREAKER" : "STAR PATROL"; }

void renderLauncher(Canvas& canvas, unsigned selection, bool browsing, uint32_t best) {
    selection = std::min(selection,1U);
    canvas.line(210,64,210,241,dim);
    for (unsigned i = 0; i < gameCount; ++i) {
        const int baseline = 91+i*34;
        if (i == selection) canvas.rectangle(8,baseline-18,192,25,browsing?ink:dim);
        canvas.text(16,baseline,name(i==0?Game::Breaker:Game::Patrol),i==selection && browsing?black:ink);
    }
    canvas.text(16,174,selection ? "SPACE SHOOTER" : "BRICK BREAKER",ink);
    char record[24];
    std::snprintf(record,sizeof(record),"BEST %lu",static_cast<unsigned long>(best));
    canvas.text(16,199,record,ink);
    canvas.text(16,226,"3 LIVES PER RUN",dim);
    if (!selection) {
        for (unsigned row = 0; row < 3; ++row) for (unsigned column = 0; column < 5; ++column)
            canvas.rectangle(239+column*38,79+row*14,32,8,row%2?dim:ink);
        canvas.rectangle(309,146,65,6,ink); canvas.rectangle(336,132,5,5,ink);
        canvas.text(228,187,"Break the brick wall.",ink);
        canvas.text(228,207,"Keep the ball in play.",ink);
        canvas.text(228,227,"Click to launch/pause.",ink);
    } else {
        for (unsigned row = 0; row < 2; ++row) for (unsigned column = 0; column < 4; ++column)
            alien(canvas,252+column*52,85+row*27,row,column%2);
        ship(canvas,330,146); canvas.rectangle(330,125,2,7,ink);
        canvas.text(228,187,"Dodge diving raiders.",ink);
        canvas.text(228,207,"Your ship auto-fires.",ink);
        canvas.text(228,227,"Click to start/pause.",ink);
    }
    canvas.text(10,265,browsing?"CLICK: PLAY  HOLD: CATEGORY":"CLICK: GAMES  TURN: SELECT",ink);
}

void render(Canvas& canvas, const State& state) {
    char hud[48];
    std::snprintf(hud,sizeof(hud),"SCORE %06lu  LIVES %u  WAVE %03u",
                  static_cast<unsigned long>(state.score),state.lives,state.level);
    canvas.text(12,68,hud,ink);
    box(canvas,10,73,461,168,dim);
    if (state.game == Game::Breaker) {
        for (unsigned i = 0; i < state.bricks.size(); ++i) if (state.bricks[i]) {
            const int x = 24+(i%brickColumns)*54, y = 90+(i/brickColumns)*16;
            canvas.rectangle(x,y,48,10,dim); box(canvas,x,y,48,10,ink);
            canvas.line(x+3,y+3,x+43,y+3,ink);
        }
        canvas.rectangle(pixel(state.playerX)-35,224,70,6,ink);
        const int x = pixel(std::max(left+3,std::min(state.ballX,right-3)));
        const int y = pixel(std::max(top+3,std::min(state.ballY,bottom-3)));
        canvas.rectangle(x-2,y-3,5,7,ink); canvas.rectangle(x-3,y-2,7,5,ink);
    } else {
        // Static stars make movement readable without spending simulation time.
        for (unsigned i = 0; i < 23; ++i) canvas.rectangle(22+(i*67)%435,82+(i*43)%147,1,1,dim);
        for (unsigned i = 0; i < state.enemies.size(); ++i) {
            const auto& enemy = state.enemies[i];
            if (enemy.active && enemy.y >= top+10 && enemy.y <= bottom-10)
                alien(canvas,pixel(enemy.x),pixel(enemy.y),i/enemyColumns,(state.ticks/140+i)%2);
        }
        for (const auto& shot : state.playerShots) if (shot.active && shot.y >= top+6 && shot.y <= bottom-4)
            canvas.rectangle(pixel(shot.x)-1,pixel(shot.y)-5,2,7,ink);
        for (const auto& shot : state.enemyShots) if (shot.active && shot.y >= top+3 && shot.y <= bottom-3)
            canvas.rectangle(pixel(shot.x)-1,pixel(shot.y)-3,3,6,ink);
        if (!state.invulnerableMs || (state.ticks/100)%2) ship(canvas,pixel(state.playerX),224);
    }
    if (state.phase != Phase::Playing) {
        canvas.rectangle(99,151,282,63,black); box(canvas,99,151,282,63,ink);
        const char* title = state.phase == Phase::Ready ? name(state.game) :
            state.phase == Phase::Paused ? "PAUSED" : state.phase == Phase::Cleared ? "WAVE CLEARED" : "GAME OVER";
        centered(canvas,174,title);
        centered(canvas,199,state.phase == Phase::Paused ? "CLICK TO RESUME" :
            state.phase == Phase::Cleared ? "CLICK FOR NEXT WAVE" : state.phase == Phase::GameOver ? "CLICK TO RESTART" :
            state.game == Game::Breaker ? "CLICK TO LAUNCH" : "CLICK TO START");
    }
    canvas.text(10,265,state.phase == Phase::Playing ? "TURN:MOVE CLICK:PAUSE HOLD:BACK" :
                "TURN:MOVE CLICK:PLAY  HOLD:BACK",ink);
}
}
