#include "Arduino.h"
#include "Audio.h"
#include "MusicPlayer.h"
#include "esp_heap_caps.h"
#include <cassert>
#include <functional>
#include <iostream>

void waitFor(const std::function<bool(const music::Snapshot&)>& predicate) {
    for (unsigned attempt = 0; attempt < 500; ++attempt) {
        if (predicate(music::snapshot())) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    const auto state = music::snapshot();
    std::cerr << "Timed out, state=" << static_cast<int>(state.state) << " error=" << state.error << '\n';
    std::abort();
}
void send(music::Action action, uint16_t track = 0) { assert(music::command(action, track)); }
void waitState(music::State expected) {
    waitFor([=](const music::Snapshot& value) { return value.state == expected; });
}
std::vector<uint8_t> tagged() {
    std::vector<uint8_t> file(128, 0);
    std::memcpy(file.data(), "TAGMetadata title", 17);
    std::memcpy(file.data() + 33, "Artist", 6);
    std::memcpy(file.data() + 63, "Album", 5);
    return file;
}
int main() {
    assert(!music::command(music::Action::Play));
    music::begin();
    waitState(music::State::NoCard);
    assert(music::snapshot().count == 0);
    SD.mounted = true;
    send(music::Action::Rescan);
    waitState(music::State::Empty);
    SD.add("/folder/alpha.MP3");
    SD.add("/tagged.mp3", tagged());
    SD.add("/bad.mp3");
    SD.add("/notes.txt");
    SD.add("/.hidden.mp3");
    send(music::Action::Rescan);
    waitState(music::State::Stopped);
    assert(music::snapshot().count == 3);
    assert(music::track(0).title == "Metadata title");
    assert(music::track(0).artist == "Artist" && music::track(0).album == "Album");
    assert(music::track(1).title == "alpha" && music::track(1).path == "/folder/alpha.MP3");
    assert(music::track(65535).title.empty());
    send(music::Action::Play, 1);
    waitFor([](const music::Snapshot& s) { return s.state == music::State::Playing && host::outputLeft > 1000; });
    assert(music::snapshot().current == 1);
    assert(music::snapshot().volume == 21 && music::snapshot().boostDb == 6);
    assert(host::outputLeft > 1000); // The actual I2S hook applies additional gain.
    send(music::Action::BoostUp);
    waitFor([](const music::Snapshot& s) { return s.boostDb == 12; });
    send(music::Action::BoostUp);
    waitFor([](const music::Snapshot& s) { return s.boostDb == 18; });
    for (unsigned db = 24; db <= 90; db += 6) {
        send(music::Action::BoostUp);
        waitFor([=](const music::Snapshot& s) { return s.boostDb == db; });
    }
    send(music::Action::BoostUp); // clamps at MAX
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(music::snapshot().boostDb == 90);
    for (int db = 84; db >= 0; db -= 6) {
        send(music::Action::BoostDown);
        waitFor([=](const music::Snapshot& s) { return s.boostDb == db; });
    }
    send(music::Action::BoostDown);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(music::snapshot().boostDb == 0 && host::outputLeft == 1000);
    // No Radio/UI rendering or service calls occur on this thread. The real
    // MusicPlayer.cpp task must keep servicing Audio independently.
    const auto loops = host::audioLoops.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(host::audioLoops > loops + 5 && music::snapshot().state == music::State::Playing);
    send(music::Action::TogglePause);
    waitState(music::State::Paused);
    send(music::Action::TogglePause);
    waitState(music::State::Playing);
    send(music::Action::Next);
    waitState(music::State::Error); // bad.mp3 fails to open
    send(music::Action::Next);
    waitFor([](const music::Snapshot& s) { return s.state == music::State::Playing && s.current == 0; });
    host::eof = true;
    waitFor([](const music::Snapshot& s) { return s.state == music::State::Playing && s.current == 1; });
    send(music::Action::Stop);
    waitState(music::State::Stopped);
    send(music::Action::Play, 60000); // stale/out-of-range selection is ignored
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(music::snapshot().state == music::State::Stopped);
    send(music::Action::Previous);
    waitFor([](const music::Snapshot& s) { return s.state == music::State::Playing && s.current == 0; });
    host::timeOffset += 6000;
    host::failDecoder = true;
    waitState(music::State::Error);
    send(music::Action::Rescan);
    waitState(music::State::Stopped);
    assert(music::snapshot().current == -1 && music::snapshot().nowPlaying.title.empty());
    host::failPsram = true;
    send(music::Action::Rescan);
    waitState(music::State::Error);
    assert(music::snapshot().error.find("PSRAM") != std::string::npos);
    host::failPsram = false;
    SD.mounted = false;
    send(music::Action::Rescan);
    waitState(music::State::NoCard);
    assert(music::snapshot().count == 0);
    SD.mounted = true;
    SD.clear();
    for (unsigned i = 0; i < 260; ++i) SD.add("/track" + std::to_string(i) + ".mp3");
    send(music::Action::Rescan);
    waitState(music::State::Stopped);
    assert(music::snapshot().count == music::maxTracks && music::snapshot().limited);
    // Volume commands are processed asynchronously and remain clamped to 0..21.
    for (unsigned i = 0; i < 25; ++i) {
        send(music::Action::VolumeUp);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    waitFor([](const music::Snapshot& s) { return s.volume == 21; });
    for (unsigned i = 0; i < 25; ++i) {
        send(music::Action::VolumeDown);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    waitFor([](const music::Snapshot& s) { return s.volume == 0; });
    for (unsigned db = 6; db <= 90; db += 6) {
        send(music::Action::BoostUp);
        waitFor([=](const music::Snapshot& s) { return s.boostDb == db; });
    }
    send(music::Action::Play, 0);
    waitFor([](const music::Snapshot& s) { return s.state == music::State::Playing && host::outputLeft == 0; });
    assert(host::outputLeft == 0); // Boost must preserve mute.
    host::stop = true;
    host::task.join();
    std::cout << "Music task: background service, controls, EOF, scanning, metadata and recovery checks passed\n";
}
