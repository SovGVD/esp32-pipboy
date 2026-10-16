#include <Arcade.h>
#include "NativeFrame.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using Frame = NativeFrame<arcade::Canvas>;
void header(Frame& frame, const char* game = nullptr) {
    constexpr uint16_t ink=0x45a2;
    frame.font=&monofonto11pt7b;
    const char* names[]={"STAT","INV","DATA","MAP","RADIO"};
    const int positions[]={60,140,210,290,360};
    for(unsigned i=0;i<5;++i)frame.fixedText(positions[i],21,names[i],ink);
    frame.line(0,27,203,27,ink);frame.line(203,27,203,11,ink);
    frame.line(254,11,258,11,ink);frame.line(258,11,258,27,ink);frame.line(258,27,470,27,ink);
    frame.fixedText(210,51,"GAMES",ink);
    frame.font=&monofonto10pt7b;
    if(game)frame.text(278,50,game,ink);
}
void save(Frame& frame, int argc, char** argv, const char* name) {
    if(argc==2)frame.save(std::string(argv[1])+"/"+name+".ppm");
}
int main(int argc,char** argv) {
    for(unsigned selected=0;selected<2;++selected){
        Frame frame;header(frame);arcade::renderLauncher(frame,selected,true);
        save(frame,argc,argv,selected?"patrol-menu":"breaker-menu");
    }
    for(auto type:{arcade::Game::Breaker,arcade::Game::Patrol}){
        arcade::Session game(type);
        Frame ready;header(ready,arcade::name(type));arcade::render(ready,game.state());
        save(ready,argc,argv,type==arcade::Game::Breaker?"breaker-ready":"patrol-ready");
        game.update(0,{0,true});
        Frame active;header(active,arcade::name(type));
        for(unsigned i=0;i<480;++i){
            const int turn=type==arcade::Game::Breaker?
                static_cast<int>(std::lround((game.state().ballX-game.state().playerX)/10)):
                (i%30==0?(i%60==0?1:-1):0);
            game.update(8,{turn,game.state().phase==arcade::Phase::Ready});
        }
        // Capture the visible half of the shooter's temporary hit-grace blink.
        while(type==arcade::Game::Patrol && game.state().phase==arcade::Phase::Playing &&
              game.state().invulnerableMs && !(game.state().ticks/100%2))game.update(8,{});
        arcade::render(active,game.state());
        save(active,argc,argv,type==arcade::Game::Breaker?"breaker-play":"patrol-play");
        for(auto phase:{arcade::Phase::Ready,arcade::Phase::Playing,arcade::Phase::Paused,arcade::Phase::Cleared,arcade::Phase::GameOver}){
            auto state=game.state();state.phase=phase;
            if(phase==arcade::Phase::GameOver)state.lives=0;
            Frame frame;header(frame,arcade::name(type));arcade::render(frame,state);
            assert(frame.lines>5 && frame.texts>5);
            if(phase==arcade::Phase::Paused)save(frame,argc,argv,type==arcade::Game::Breaker?"breaker-paused":"patrol-paused");
            if(phase==arcade::Phase::GameOver)save(frame,argc,argv,type==arcade::Game::Breaker?"breaker-over":"patrol-over");
            state.score=std::numeric_limits<uint32_t>::max();state.level=999;
            Frame maximum;header(maximum,arcade::name(type));arcade::render(maximum,state);
        }
    }
    // Every primitive remains inside the native canvas even at the playfield edges.
    for(auto type:{arcade::Game::Breaker,arcade::Game::Patrol}){
        arcade::Session game(type);
        for(unsigned n=0;n<15000;++n){
            const bool click=game.state().phase!=arcade::Phase::Playing;
            game.update(32,{n%20<10?1:-1,click});
            if(n%13==0){Frame frame;arcade::render(frame,game.state());}
        }
    }
    std::cout<<"Arcade: launcher, all phases, maximum score and native text/pixel bounds passed\n";
}
