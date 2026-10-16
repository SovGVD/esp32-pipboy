#include "Arcade.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace arcade {
namespace {
constexpr float dt = 0.008f;
constexpr float radius = 3;
constexpr float paddleHalf = 35, paddleY = 224;
float clamp(float value, float minimum, float maximum) { return std::max(minimum, std::min(value, maximum)); }
unsigned difficulty(const State& state) { return std::min(state.level, 10U); }
float ballSpeed(const State& state) { return 176 + difficulty(state) * 12; }
float homeX(unsigned index) { return 50 + (index % enemyColumns) * 72; }
float homeY(unsigned index) { return 94 + (index / enemyColumns) * 22; }
bool circleBox(float x, float y, float bx, float by, float width, float height) {
    const float dx = x - clamp(x, bx, bx+width), dy = y - clamp(y, by, by+height);
    return dx*dx + dy*dy <= radius*radius;
}
}

Session::Session(Game game) { state_.game = game; restart(); }

void Session::restart() {
    const auto game = state_.game;
    const auto best = state_.best;
    state_ = State{};
    state_.game = game;
    state_.best = best;
    random_ = 0x5eed1234;
    round();
}

void Session::round() {
    state_.phase = Phase::Ready;
    resume_ = Phase::Ready;
    remainder_ = fireMs_ = enemyFireMs_ = diveMs_ = 0;
    state_.ticks = 0;
    state_.invulnerableMs = 0;
    state_.playerX = 240;
    state_.ballX = 240;
    state_.ballY = paddleY - radius - 2;
    state_.ballVx = ballSpeed(state_) * 0.42f;
    state_.ballVy = -std::sqrt(ballSpeed(state_)*ballSpeed(state_) - state_.ballVx*state_.ballVx);
    state_.bricks.fill(true);
    for (unsigned i = 0; i < state_.enemies.size(); ++i) {
        state_.enemies[i] = Enemy{};
        state_.enemies[i].x = homeX(i);
        state_.enemies[i].y = homeY(i);
    }
    state_.playerShots.fill(Shot{});
    state_.enemyShots.fill(Shot{});
}

void Session::pause() {
    if (state_.phase == Phase::Playing) {
        resume_ = state_.phase;
        state_.phase = Phase::Paused;
    }
    remainder_ = 0;
}

void Session::update(uint32_t elapsedMs, Input input) {
    const bool attached = state_.game == Game::Breaker && state_.phase == Phase::Ready;
    if (input.click) {
        remainder_ = 0;
        if (state_.phase == Phase::Playing) pause();
        else if (state_.phase == Phase::Paused) state_.phase = resume_;
        else if (state_.phase == Phase::GameOver) restart();
        else if (state_.phase == Phase::Cleared) {
            if (state_.level < 999) ++state_.level;
            round();
        } else if (state_.phase == Phase::Ready) state_.phase = Phase::Playing;
        // Never simulate pre-click elapsed time after launching or resuming.
        elapsedMs = 0;
    }
    if (state_.phase == Phase::Ready || state_.phase == Phase::Playing) {
        const int turn = std::max(-100, std::min(input.turn, 100));
        const float margin = state_.game == Game::Breaker ? paddleHalf + 1 : 13;
        state_.playerX = clamp(state_.playerX + turn * 10, left + margin, right - margin);
        if (state_.game == Game::Breaker && (attached || state_.phase == Phase::Ready)) state_.ballX = state_.playerX;
    }
    if (state_.phase != Phase::Playing) { remainder_ = 0; return; }
    remainder_ += std::min(elapsedMs, uint32_t{96});
    while (remainder_ >= 8 && state_.phase == Phase::Playing) {
        remainder_ -= 8;
        step();
    }
    if (state_.phase != Phase::Playing) remainder_ = 0;
}

void Session::step() {
    state_.ticks += 8;
    if (state_.game == Game::Breaker) breaker();
    else patrol();
}

void Session::addScore(unsigned points) {
    state_.score += std::min(points, std::numeric_limits<uint32_t>::max() - state_.score);
    state_.best = std::max(state_.best, state_.score);
}

void Session::breaker() {
    const float oldX = state_.ballX, oldY = state_.ballY;
    float& x = state_.ballX;
    float& y = state_.ballY;
    x += state_.ballVx * dt;
    y += state_.ballVy * dt;
    if (x < left+radius) { x = left+radius; state_.ballVx = std::abs(state_.ballVx); }
    if (x > right-radius) { x = right-radius; state_.ballVx = -std::abs(state_.ballVx); }
    if (y < top+radius) { y = top+radius; state_.ballVy = std::abs(state_.ballVy); }
    if (state_.ballVy > 0 && oldY+radius <= paddleY && y+radius >= paddleY &&
        x >= state_.playerX-paddleHalf-radius && x <= state_.playerX+paddleHalf+radius) {
        const float speed = ballSpeed(state_);
        const float offset = clamp((x-state_.playerX)/paddleHalf, -1, 1);
        state_.ballVx = offset * speed * 0.85f;
        if (std::abs(state_.ballVx) < 24) state_.ballVx = state_.ballVx < 0 ? -24 : 24;
        state_.ballVy = -std::sqrt(speed*speed-state_.ballVx*state_.ballVx);
        y = paddleY-radius-0.01f;
    }
    for (unsigned i = 0; i < state_.bricks.size(); ++i) {
        if (!state_.bricks[i]) continue;
        const float bx = 24+(i%brickColumns)*54, by = 90+(i/brickColumns)*16;
        if (!circleBox(x,y,bx,by,48,10)) continue;
        state_.bricks[i] = false;
        addScore(10*(brickRows-i/brickColumns));
        if (oldY+radius <= by) { y = by-radius-0.01f; state_.ballVy = -std::abs(state_.ballVy); }
        else if (oldY-radius >= by+10) { y = by+10+radius+0.01f; state_.ballVy = std::abs(state_.ballVy); }
        else if (oldX+radius <= bx) { x = bx-radius-0.01f; state_.ballVx = -std::abs(state_.ballVx); }
        else {
            // A circular corner contact may start inside the expanded AABB.
            // Resolve its nearest face instead of teleporting across the brick.
            const float distances[] = {std::abs(x-(bx-radius)),std::abs(x-(bx+48+radius)),
                                       std::abs(y-(by-radius)),std::abs(y-(by+10+radius))};
            const unsigned face = std::min_element(distances,distances+4)-distances;
            if (face == 0) { x = bx-radius-0.01f; state_.ballVx = -std::abs(state_.ballVx); }
            else if (face == 1) { x = bx+48+radius+0.01f; state_.ballVx = std::abs(state_.ballVx); }
            else if (face == 2) { y = by-radius-0.01f; state_.ballVy = -std::abs(state_.ballVy); }
            else { y = by+10+radius+0.01f; state_.ballVy = std::abs(state_.ballVy); }
        }
        break; // One resolved contact per substep avoids reflecting twice in a seam.
    }
    if (std::none_of(state_.bricks.begin(),state_.bricks.end(),[](bool brick){return brick;})) {
        state_.phase = Phase::Cleared;
        return;
    }
    if (y-radius > bottom) {
        if (--state_.lives == 0) state_.phase = Phase::GameOver;
        else {
            state_.phase = Phase::Ready;
            state_.ballX = state_.playerX;
            state_.ballY = paddleY-radius-2;
            state_.ballVx = ballSpeed(state_)*0.42f;
            state_.ballVy = -std::sqrt(ballSpeed(state_)*ballSpeed(state_)-state_.ballVx*state_.ballVx);
        }
    }
}

unsigned Session::random(unsigned maximum) {
    random_ = random_*1664525U + 1013904223U;
    return maximum ? random_%maximum : 0;
}

void Session::hitPlayer() {
    if (state_.invulnerableMs || state_.phase != Phase::Playing) return;
    if (--state_.lives == 0) { state_.phase = Phase::GameOver; return; }
    state_.invulnerableMs = 1400;
    state_.playerX = 240;
    state_.enemyShots.fill(Shot{});
}

void Session::patrol() {
    if (state_.invulnerableMs) state_.invulnerableMs -= std::min(state_.invulnerableMs,8U);
    const float clock = state_.ticks * 0.001f;
    for (unsigned i = 0; i < state_.enemies.size(); ++i) {
        auto& enemy = state_.enemies[i];
        if (!enemy.active) continue;
        if (enemy.diving) {
            enemy.diveTime += dt;
            enemy.x += clamp(enemy.target-enemy.x,-70,70)*dt + std::cos(enemy.diveTime*5)*45*dt;
            enemy.x = clamp(enemy.x,left+12,right-12);
            enemy.y += (65+difficulty(state_)*5)*dt;
            if (enemy.y > bottom+12) { enemy.diving = false; enemy.diveTime = 0; }
        } else {
            enemy.x = homeX(i)+std::sin(clock*0.9f)*20;
            enemy.y = homeY(i)+std::sin(clock*0.7f)*3;
        }
    }
    fireMs_ += 8;
    if (fireMs_ >= 200) {
        fireMs_ = 0;
        for (auto& shot : state_.playerShots) if (!shot.active) {
            shot.active = true; shot.x = state_.playerX; shot.y = 209; shot.vx = 0; shot.vy = -225;
            break;
        }
    }
    diveMs_ += 8;
    if (diveMs_ >= 2600-std::min(difficulty(state_),8U)*150) {
        diveMs_ = 0;
        if (std::none_of(state_.enemies.begin(),state_.enemies.end(),[](const Enemy& e){return e.active && e.diving;})) {
            const unsigned first = random(state_.enemies.size());
            for (unsigned n = 0; n < state_.enemies.size(); ++n) {
                auto& enemy = state_.enemies[(first+n)%state_.enemies.size()];
                if (!enemy.active) continue;
                enemy.diving = true; enemy.target = state_.playerX; enemy.diveTime = 0;
                break;
            }
        }
    }
    enemyFireMs_ += 8;
    if (enemyFireMs_ >= 1000-difficulty(state_)*50) {
        enemyFireMs_ = 0;
        const unsigned first = random(state_.enemies.size());
        for (unsigned n = 0; n < state_.enemies.size(); ++n) {
            const auto& enemy = state_.enemies[(first+n)%state_.enemies.size()];
            if (!enemy.active) continue;
            for (auto& shot : state_.enemyShots) if (!shot.active) {
                shot.active = true; shot.x = enemy.x; shot.y = enemy.y+8;
                shot.vx = clamp((state_.playerX-enemy.x)*0.25f,-40,40);
                shot.vy = 90+difficulty(state_)*5;
                break;
            }
            break;
        }
    }
    for (auto& shot : state_.playerShots) {
        if (!shot.active) continue;
        shot.x += shot.vx*dt; shot.y += shot.vy*dt;
        if (shot.y < top+2) { shot.active = false; continue; }
        for (unsigned i = 0; i < state_.enemies.size(); ++i) {
            auto& enemy = state_.enemies[i];
            if (enemy.active && std::abs(shot.x-enemy.x) <= 11 && std::abs(shot.y-enemy.y) <= 10) {
                shot.active = false; enemy.active = false;
                addScore(25*(enemyRows-i/enemyColumns)+(enemy.diving?50:0));
                break;
            }
        }
    }
    for (auto& shot : state_.enemyShots) {
        if (!shot.active) continue;
        shot.x += shot.vx*dt; shot.y += shot.vy*dt;
        if (shot.x < left+2 || shot.x > right-2 || shot.y > bottom-3) { shot.active = false; continue; }
        if (std::abs(shot.x-state_.playerX) <= 12 && shot.y >= 212 && shot.y <= 236) {
            shot.active = false;
            hitPlayer();
        }
    }
    for (auto& enemy : state_.enemies) {
        if (enemy.active && enemy.diving && std::abs(enemy.x-state_.playerX) < 23 && enemy.y > 207 && enemy.y < 239) {
            enemy.diving = false;
            hitPlayer();
        }
    }
    if (state_.phase == Phase::Playing && std::none_of(state_.enemies.begin(),state_.enemies.end(),[](const Enemy& e){return e.active;}))
        state_.phase = Phase::Cleared;
}
}
