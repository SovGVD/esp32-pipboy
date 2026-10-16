#pragma once

#include <cstdint>
#include <string>

namespace music {
constexpr uint16_t maxTracks = 256;
struct Track {
    std::string path;
    std::string title;
    std::string artist;
    std::string album;
};
enum class State { Starting, NoCard, Empty, Stopped, Playing, Paused, Error };
enum class Action { Play, TogglePause, Stop, Next, Previous, VolumeUp, VolumeDown, BoostUp, BoostDown, Rescan };
struct Snapshot {
    State state = State::Starting;
    uint16_t count = 0;
    int current = -1;
    uint8_t volume = 0;
    uint8_t boostDb = 0;
    uint32_t elapsed = 0;
    uint32_t duration = 0;
    bool limited = false;
    std::string error;
    Track nowPlaying;
};

void begin();
bool command(Action action, uint16_t track = 0);
Snapshot snapshot();
Track track(uint16_t index);
}
