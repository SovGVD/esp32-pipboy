#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace arcade {
constexpr float left = 12, right = 468, top = 74, bottom = 239;
constexpr unsigned brickColumns = 8, brickRows = 4;
constexpr unsigned enemyColumns = 6, enemyRows = 3;
constexpr size_t gameCount = 2;
enum class Game : uint8_t { Breaker, Patrol };
enum class Phase : uint8_t { Ready, Playing, Paused, Cleared, GameOver };
struct Input {
    int turn;
    bool click;
    Input(int turn = 0, bool click = false) : turn(turn), click(click) {}
};
struct Shot { float x = 0, y = 0, vx = 0, vy = 0; bool active = false; };
struct Enemy { float x = 0, y = 0, target = 0, diveTime = 0; bool active = true, diving = false; };
struct State {
    Game game = Game::Breaker;
    Phase phase = Phase::Ready;
    uint32_t score = 0, best = 0, ticks = 0;
    unsigned level = 1, lives = 3;
    float playerX = 240;
    float ballX = 240, ballY = 216, ballVx = 80, ballVy = -160;
    std::array<bool, brickColumns * brickRows> bricks{};
    std::array<Enemy, enemyColumns * enemyRows> enemies{};
    std::array<Shot, 8> playerShots{};
    std::array<Shot, 10> enemyShots{};
    unsigned invulnerableMs = 0;
};

// Simulation owns no hardware, heap allocations, display, audio or wall clock.
// Elapsed time is subdivided into 8 ms steps and capped at 96 ms per update.
class Session {
public:
    explicit Session(Game game = Game::Breaker);
    const State& state() const { return state_; }
    void update(uint32_t elapsedMs, Input input);
    void pause();
    void restart();
private:
    friend struct TestAccess;
    State state_;
    unsigned remainder_ = 0, fireMs_ = 0, enemyFireMs_ = 0, diveMs_ = 0;
    uint32_t random_ = 0x5eed1234;
    Phase resume_ = Phase::Ready;
    void round();
    void step();
    void breaker();
    void patrol();
    void hitPlayer();
    void addScore(unsigned points);
    unsigned random(unsigned maximum);
};

class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void line(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) = 0;
    virtual void rectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void text(int x, int baseline, const char* text, uint16_t color) = 0;
};
const char* name(Game game);
void renderLauncher(Canvas& canvas, unsigned selection, bool browsing, uint32_t best = 0);
void render(Canvas& canvas, const State& state);
}
