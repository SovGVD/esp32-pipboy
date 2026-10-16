#pragma once
#include "SD.h"
#include <cassert>
extern void audio_eof_mp3(const char*);
extern void audio_process_i2s(uint32_t*, bool*);
namespace host {
inline std::atomic<unsigned> audioLoops{0};
inline std::atomic<bool> eof{false};
inline std::atomic<bool> failDecoder{false};
inline std::atomic<int> outputLeft{0};
}
class Audio {
public:
    Audio() : owner_(std::this_thread::get_id()) {}
    bool setPinout(int, int, int) { check(); return true; }
    void setVolume(uint8_t volume) { check(); volume_ = volume; }
    void stopSong() { check(); running_ = connected_ = false; }
    bool connecttoFS(SDClass&, const char* path) {
        check(); connected_ = running_ = !std::strstr(path, "bad"); return connected_;
    }
    bool pauseResume() {
        check();
        if (!connected_) return false;
        running_ = !running_;
        return true; // Real library reports success, not the new running state.
    }
    bool isRunning() { check(); return running_; }
    void loop() {
        check(); ++host::audioLoops;
        if (running_ && host::eof.exchange(false)) { running_ = false; audio_eof_mp3(""); }
        if (host::failDecoder.exchange(false)) running_ = false;
        if (running_) {
            const int32_t level = 1000 * volume_ * volume_ / (21 * 21);
            uint32_t frame = (static_cast<uint32_t>(level) << 16) | static_cast<uint32_t>(level);
            bool send = false;
            audio_process_i2s(&frame, &send);
            assert(send); // An unset continuation flag would silently discard music.
            host::outputLeft = static_cast<int16_t>(frame >> 16);
        }
    }
    uint32_t getAudioCurrentTime() { check(); return running_ ? 3 : 0; }
    uint32_t getAudioFileDuration() { check(); return connected_ ? 120 : 0; }
    uint32_t getSampleRate() { check(); return 44100; }
private:
    void check() { assert(owner_ == std::this_thread::get_id()); }
    std::thread::id owner_;
    bool running_ = false, connected_ = false;
    uint8_t volume_ = 0;
};
