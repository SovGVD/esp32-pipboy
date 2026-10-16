#include "MusicPlayer.h"
#include "Hardware.h"
#include <Arduino.h>
#include <Audio.h>
#include <AudioBoost.h>
#include <Id3Metadata.h>
#include <SD.h>
#include <SPI.h>
#include <algorithm>
#include <atomic>
#include <memory>
#include <cstring>
#include <esp_heap_caps.h>

namespace {
struct Command { music::Action action; uint16_t track; };
QueueHandle_t commands = nullptr;
SemaphoreHandle_t guard = nullptr;
music::Snapshot status;
// Store the bounded catalog in PSRAM as fixed records. A large card cannot
// exhaust internal RAM with hundreds of separately allocated metadata strings.
struct CachedTrack {
    char path[256];
    char title[193];
    char artist[193];
    char album[193];
    music::Track copy() const { return {path, title, artist, album}; }
};
struct FreeCatalog { void operator()(CachedTrack* ptr) const { heap_caps_free(ptr); } };
using Catalog = std::unique_ptr<CachedTrack, FreeCatalog>;
Catalog tracks;
uint16_t trackCount = 0;
std::atomic<bool> ended{false};
audio::Boost outputBoost; // Accessed only by the audio task and its I2S hook.

class FileReader : public id3::Reader {
public:
    explicit FileReader(File& file) : file_(file), size_(file.size()) {}
    uint32_t size() const override { return size_; }
    bool read(uint32_t offset, uint8_t* data, size_t count) override {
        return offset <= size_ && count <= size_ - offset && file_.seek(offset) &&
               file_.read(data, count) == count;
    }
private:
    File& file_;
    uint32_t size_;
};

bool isMp3(const std::string& path) {
    if (path.size() < 4 || path[path.size() - 4] != '.') return false;
    const auto* ext = path.c_str() + path.size() - 3;
    return (ext[0] == 'm' || ext[0] == 'M') && (ext[1] == 'p' || ext[1] == 'P') && ext[2] == '3';
}

void scanDirectory(File& dir, unsigned depth, CachedTrack* found, uint16_t& count,
                   unsigned& entries, bool& limited) {
    while (true) {
        File file = dir.openNextFile();
        if (!file) break;
        if (++entries > 4096 || count == music::maxTracks) { limited = true; break; }
        const std::string name = file.name();
        if (!name.empty() && name[0] != '.') {
            if (file.isDirectory()) {
                if (depth < 4) scanDirectory(file, depth + 1, found, count, entries, limited);
                else limited = true;
            } else if (isMp3(name)) {
                FileReader reader(file);
                const auto tags = id3::read(reader);
                const char* path = file.path();
                if (strlen(path) >= sizeof(CachedTrack::path)) { limited = true; continue; }
                auto& track = found[count++];
                snprintf(track.path, sizeof(track.path), "%s", path);
                snprintf(track.title, sizeof(track.title), "%s",
                         tags.title.empty() ? name.substr(0, name.size() - 4).c_str() : tags.title.c_str());
                snprintf(track.artist, sizeof(track.artist), "%s", tags.artist.c_str());
                snprintf(track.album, sizeof(track.album), "%s", tags.album.c_str());
            }
        }
        file.close();
        vTaskDelay(1);
        if (entries > 4096 || count == music::maxTracks) { limited = true; break; }
    }
}

void scan(Audio& audio) {
    audio.stopSong();
    ended = false;
    xSemaphoreTake(guard, portMAX_DELAY);
    status.state = music::State::Starting;
    status.current = -1;
    status.elapsed = status.duration = 0;
    status.nowPlaying = {};
    status.error.clear();
    status.count = 0;
    status.limited = false;
    tracks.reset();
    trackCount = 0;
    xSemaphoreGive(guard);

    SD.end();
    if (!SD.begin(hardware::sdCs, SPI, 10000000) || SD.cardType() == CARD_NONE) {
        xSemaphoreTake(guard, portMAX_DELAY);
        status.state = music::State::NoCard;
        status.error = "Insert FAT32 SD; select RESCAN";
        xSemaphoreGive(guard);
        return;
    }
    Catalog found(static_cast<CachedTrack*>(heap_caps_calloc(music::maxTracks, sizeof(CachedTrack),
                                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
    if (!found) {
        xSemaphoreTake(guard, portMAX_DELAY);
        status.state = music::State::Error;
        status.error = "PSRAM unavailable for music library";
        xSemaphoreGive(guard);
        return;
    }
    uint16_t count = 0;
    unsigned entries = 0;
    bool limited = false;
    File root = SD.open("/");
    const bool rootOk = root && root.isDirectory();
    if (rootOk) scanDirectory(root, 0, found.get(), count, entries, limited);
    root.close();
    std::sort(found.get(), found.get() + count, [](const CachedTrack& a, const CachedTrack& b) {
        const int titleOrder = strcmp(a.title, b.title);
        return titleOrder == 0 ? strcmp(a.path, b.path) < 0 : titleOrder < 0;
    });
    xSemaphoreTake(guard, portMAX_DELAY);
    tracks.swap(found);
    trackCount = count;
    status.count = count;
    status.limited = limited;
    status.state = !rootOk ? music::State::Error : !count ? music::State::Empty : music::State::Stopped;
    if (!rootOk) status.error = "Cannot read SD root; select RESCAN";
    xSemaphoreGive(guard);
}

void playerTask(void*) {
    // This task exclusively owns Audio, SD, and all decoder calls. UI threads
    // exchange commands and copies of state; they never touch the decoder.
    std::unique_ptr<Audio> audio(new Audio());
    SPI.begin(hardware::sdSck, hardware::sdMiso, hardware::sdMosi, hardware::sdCs);
    if (!audio->setPinout(hardware::audioBclk, hardware::audioLrclk, hardware::audioData)) {
        xSemaphoreTake(guard, portMAX_DELAY);
        status.state = music::State::Error;
        status.error = "Audio output initialization failed";
        xSemaphoreGive(guard);
        audio.reset();
        vTaskDelete(nullptr);
        return;
    }
    uint8_t volume = hardware::audioVolume;
    outputBoost.setLevel(hardware::audioBoostLevel);
    audio->setVolume(volume);
    scan(*audio);
    int current = -1;
    music::State state = trackCount == 0 ? music::State::Empty : music::State::Stopped;
    uint32_t started = 0, published = 0;

    auto play = [&](int index) {
        if (index < 0 || index >= static_cast<int>(trackCount)) return;
        audio->stopSong();
        ended = false;
        outputBoost.reset();
        current = index;
        const bool ok = audio->connecttoFS(SD, tracks.get()[index].path);
        state = ok ? music::State::Playing : music::State::Error;
        started = millis();
        xSemaphoreTake(guard, portMAX_DELAY);
        status.current = current;
        status.nowPlaying = tracks.get()[index].copy();
        status.error = ok ? "" : "Cannot open track; choose another";
        status.state = state;
        status.elapsed = status.duration = 0;
        xSemaphoreGive(guard);
    };

    for (;;) {
        Command cmd;
        while (xQueueReceive(commands, &cmd, 0) == pdTRUE) {
            switch (cmd.action) {
            case music::Action::Play: play(cmd.track); break;
            case music::Action::TogglePause:
                if (state == music::State::Playing || state == music::State::Paused) {
                    if (audio->pauseResume()) {
                        state = audio->isRunning() ? music::State::Playing : music::State::Paused;
                    }
                } else if (trackCount != 0) play(current < 0 ? 0 : current);
                break;
            case music::Action::Stop:
                audio->stopSong(); ended = false;
                if (trackCount != 0) state = music::State::Stopped;
                break;
            case music::Action::Next:
                if (trackCount != 0) play((current + 1) % trackCount);
                break;
            case music::Action::Previous:
                if (trackCount != 0) play(current <= 0 ? trackCount - 1 : current - 1);
                break;
            case music::Action::VolumeUp: if (volume < 21) audio->setVolume(++volume); break;
            case music::Action::VolumeDown: if (volume > 0) audio->setVolume(--volume); break;
            case music::Action::BoostUp:
                if (outputBoost.level() < audio::Boost::maxLevel) outputBoost.setLevel(outputBoost.level() + 1);
                break;
            case music::Action::BoostDown:
                if (outputBoost.level() > 0) outputBoost.setLevel(outputBoost.level() - 1);
                break;
            case music::Action::Rescan:
                scan(*audio);
                current = -1;
                state = trackCount == 0 ? music::State::Empty : music::State::Stopped;
                break;
            }
        }
        audio->loop();
        if (ended.exchange(false) && state == music::State::Playing && trackCount != 0) {
            play((current + 1) % trackCount);
        }
        if (state == music::State::Playing && !audio->isRunning() && millis() - started > 5000) {
            state = music::State::Error;
            xSemaphoreTake(guard, portMAX_DELAY);
            status.error = "Playback failed; choose track / RESCAN";
            xSemaphoreGive(guard);
        }
        if (millis() - published >= 100) {
            outputBoost.setSampleRate(audio->getSampleRate());
            xSemaphoreTake(guard, portMAX_DELAY);
            // Preserve scan errors (NoCard/Empty/Error) until a track is chosen.
            if (current >= 0) status.state = state;
            status.volume = volume;
            status.boostDb = outputBoost.decibels();
            status.elapsed = audio->getAudioCurrentTime();
            status.duration = audio->getAudioFileDuration();
            xSemaphoreGive(guard);
            published = millis();
        }
        vTaskDelay(1);
    }
}
}

namespace music {
void begin() {
    if (guard) return;
    guard = xSemaphoreCreateMutex();
    commands = xQueueCreate(8, sizeof(Command));
    status.volume = hardware::audioVolume;
    status.boostDb = hardware::audioBoostLevel * 6;
    if (!guard || !commands || xTaskCreatePinnedToCore(playerTask, "sd-music", 12288,
                                                     nullptr, 1, nullptr, 0) != pdPASS) {
        status.state = State::Error;
        status.error = "Cannot start audio task";
        Serial.println(status.error.c_str());
        if (commands) { vQueueDelete(commands); commands = nullptr; }
    }
}

bool command(Action action, uint16_t track) {
    const Command cmd{action, track};
    return commands && xQueueSend(commands, &cmd, 0) == pdTRUE;
}

Snapshot snapshot() {
    if (!guard) return status;
    xSemaphoreTake(guard, portMAX_DELAY);
    const auto copy = status;
    xSemaphoreGive(guard);
    return copy;
}

Track track(uint16_t index) {
    if (!guard) return {};
    xSemaphoreTake(guard, portMAX_DELAY);
    const auto copy = index < trackCount ? tracks.get()[index].copy() : Track{};
    xSemaphoreGive(guard);
    return copy;
}
}

// Called by ESP32-audioI2S inside audio.loop(). Advancing tracks happens only
// after that call returns, so the decoder never re-enters itself.
void audio_eof_mp3(const char*) { ended = true; }
void audio_info(const char* message) { Serial.printf("Audio: %s\n", message); }

// The library calls this for each final stereo frame, after volume/EQ and before
// I2S output. Its continuation flag defaults to false: always opt into playback.
void audio_process_i2s(uint32_t* sample, bool* continueI2S) {
    *sample = outputBoost.process(*sample);
    *continueI2S = true;
}
