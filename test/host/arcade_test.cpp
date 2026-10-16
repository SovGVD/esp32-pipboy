#include <Arcade.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

namespace arcade {
struct TestAccess { static State& state(Session& game) { return game.state_; } };
}
using namespace arcade;
void launch(Session& game) { game.update(0,{0,true}); assert(game.state().phase == Phase::Playing); }
void same(const State& a, const State& b) {
    assert(a.score == b.score && a.lives == b.lives && a.ticks == b.ticks && a.phase == b.phase);
    assert(a.ballX == b.ballX && a.ballY == b.ballY && a.ballVx == b.ballVx && a.ballVy == b.ballVy);
    assert(a.playerX == b.playerX && a.bricks == b.bricks);
    for (unsigned i = 0; i < a.enemies.size(); ++i) {
        assert(a.enemies[i].x == b.enemies[i].x && a.enemies[i].y == b.enemies[i].y);
        assert(a.enemies[i].active == b.enemies[i].active && a.enemies[i].diving == b.enemies[i].diving);
    }
    for (unsigned i = 0; i < a.playerShots.size(); ++i)
        assert(a.playerShots[i].active == b.playerShots[i].active && a.playerShots[i].y == b.playerShots[i].y);
}
int main() {
    for (auto type : {Game::Breaker,Game::Patrol}) {
        Session game(type);
        assert(game.state().phase == Phase::Ready && game.state().lives == 3 && game.state().level == 1);
        game.update(99999,{1000000,false});
        assert(game.state().playerX < right && game.state().ticks == 0);
        game.update(0,{-1000000,false});
        assert(game.state().playerX > left);
        launch(game);
        game.update(32,{3,false});
        const auto beforePause = game.state();
        game.pause();
        const auto paused = game.state();
        assert(paused.phase == Phase::Paused);
        game.update(std::numeric_limits<uint32_t>::max(),{90,false});
        same(paused,game.state());
        game.update(999999,{0,true});
        assert(game.state().phase == Phase::Playing && game.state().ticks == beforePause.ticks);
        Session batch(type), partitioned(type);
        launch(batch); launch(partitioned);
        for (unsigned i = 0; i < 60; ++i) {
            batch.update(96,{});
            for (unsigned n = 0; n < 12; ++n) partitioned.update(8,{});
        }
        same(batch.state(),partitioned.state());
        Session capped(type); launch(capped); capped.update(999999,{});
        assert(capped.state().ticks == 96);
    }
    { // All encoder detents apply; simultaneous dial + launch serves at the paddle.
        Session game;
        game.update(0,{4,true});
        assert(game.state().playerX == 280 && game.state().ballX == 280);
    }
    { // Wall reflection and a single brick contact award the correct row score.
        Session game; launch(game); auto& state = TestAccess::state(game);
        state.ballX = left+3.1f; state.ballY = 190; state.ballVx = -100; state.ballVy = -100;
        game.update(8,{}); assert(state.ballVx > 0 && state.ballX >= left+3);
        state.bricks.fill(false); state.bricks[0] = state.bricks[7] = true;
        state.ballX = 48; state.ballY = 104; state.ballVx = 0; state.ballVy = -188;
        game.update(8,{});
        assert(!state.bricks[0] && state.score == 40 && state.ballVy > 0);
        game.update(8,{}); assert(state.score == 40);
    }
    { // Glancing corner hits resolve locally rather than jumping across a brick.
        Session game; launch(game); auto& state = TestAccess::state(game);
        state.ballX = 22.8f; state.ballY = 87.2f; state.ballVx = 100; state.ballVy = 100;
        game.update(8,{});
        assert(!state.bricks[0]);
        assert(std::abs(state.ballX-22.8f) < 5 && std::abs(state.ballY-87.2f) < 5);
    }
    { // Paddle returns the ball upward; a miss costs one life and waits for launch.
        Session game; launch(game); auto& state = TestAccess::state(game);
        state.ballX = state.playerX+20; state.ballY = 220; state.ballVx = 0; state.ballVy = 180;
        game.update(8,{}); assert(state.ballVy < 0 && state.ballVx > 0);
        state.ballX = 50; state.ballY = bottom+4; state.ballVy = 180;
        game.update(96,{}); assert(state.lives == 2 && state.phase == Phase::Ready);
        const auto tick = state.ticks; game.update(0,{0,true}); assert(state.ticks == tick);
        for (unsigned life = 0; life < 2; ++life) {
            state.ballY = bottom+4; state.ballVy = 180;
            game.update(8,{});
            if (state.phase == Phase::Ready) game.update(0,{0,true});
        }
        assert(state.lives == 0 && state.phase == Phase::GameOver);
        const auto best = state.best;
        game.update(0,{0,true}); assert(state.lives == 3 && state.score == 0 && state.best == best);
    }
    { // Clearing the last brick advances a fresh, harder board after confirmation.
        Session game; launch(game); auto& state = TestAccess::state(game);
        state.bricks.fill(false); state.bricks[0] = true;
        state.ballX = 48; state.ballY = 104; state.ballVx = 0; state.ballVy = -188;
        game.update(8,{}); assert(state.phase == Phase::Cleared);
        game.update(0,{0,true});
        assert(state.level == 2 && state.score == 40 && state.best == 40 && state.phase == Phase::Ready);
        assert(std::all_of(state.bricks.begin(),state.bricks.end(),[](bool b){return b;}));
        game.restart(); assert(state.score == 0 && state.best == 40);
    }
    { // Shooting is automatic, and live enemies eventually fire and dive.
        Session game(Game::Patrol); launch(game);
        bool shot = false, dive = false, enemyShot = false;
        for (unsigned i = 0; i < 500; ++i) {
            game.update(8,{});
            for (const auto& s : game.state().playerShots) shot |= s.active;
            for (const auto& s : game.state().enemyShots) enemyShot |= s.active;
            for (const auto& e : game.state().enemies) dive |= e.active && e.diving;
        }
        assert(shot && dive && enemyShot);
    }
    { // Player bullet contact, wave completion, hit grace period and game-over.
        Session game(Game::Patrol); launch(game); auto& state = TestAccess::state(game);
        state.enemies.fill(Enemy{});
        for (auto& enemy : state.enemies) enemy.active = false;
        state.enemies[0].active = true;
        state.playerShots[0].active = true; state.playerShots[0].x = 50; state.playerShots[0].y = 99; state.playerShots[0].vy = -225;
        game.update(8,{}); assert(state.phase == Phase::Cleared && state.score == 75);
        game.update(0,{0,true}); assert(state.level == 2 && state.phase == Phase::Ready);
        launch(game);
        auto hit = [&]() {
            auto& bullet = state.enemyShots[0]; bullet.active = true;
            bullet.x = state.playerX; bullet.y = 216; bullet.vy = 100; bullet.vx = 0;
            game.update(8,{});
        };
        hit(); assert(state.lives == 2 && state.invulnerableMs > 0);
        hit(); assert(state.lives == 2);
        state.invulnerableMs = 0; hit(); assert(state.lives == 1);
        state.invulnerableMs = 0; hit(); assert(state.lives == 0 && state.phase == Phase::GameOver);
    }
    { // Long seeded runs vary input, pause/restart and exercise bounded pools.
        std::mt19937 random(2026);
        for (auto type : {Game::Breaker,Game::Patrol}) {
            Session game(type);
            for (unsigned tick = 0; tick < 50000; ++tick) {
                const auto phase = game.state().phase;
                const bool click = phase != Phase::Playing || random()%200 == 0;
                game.update(random()%130,{static_cast<int>(random()%7)-3,click});
                const auto& state = game.state();
                assert(state.playerX > left && state.playerX < right && state.lives <= 3);
                assert(std::isfinite(state.ballX) && std::isfinite(state.ballY));
                for (const auto& enemy : state.enemies) assert(std::isfinite(enemy.x) && std::isfinite(enemy.y));
            }
        }
    }
    std::cout << "Arcade: fixed-step timing, full dial input, collisions, lives, waves, diving/fire, pause and seeded endurance checks passed\n";
}
