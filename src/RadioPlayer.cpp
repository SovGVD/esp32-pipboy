#include "Pipboy.h"
#include "MusicPlayer.h"
#include <algorithm>

namespace {
constexpr uint16_t controlCount = 10;
constexpr unsigned visibleRows = 5;

// The bundled Fallout font has ASCII glyphs. Replace each unsupported codepoint
// once and cap every line so long metadata cannot draw over adjacent controls.
String displayText(const std::string& text, size_t columns) {
    String out;
    for (unsigned char c : text) {
        if ((c & 0xc0) == 0x80) continue;
        if (out.length() == columns) {
            if (columns > 2) { out.remove(columns - 2); out += ".."; }
            break;
        }
        out += (c >= 32 && c < 127) ? static_cast<char>(c) : '?';
    }
    return out;
}

const char* stateName(music::State state) {
    switch (state) {
    case music::State::Starting: return "SCANNING SD";
    case music::State::NoCard: return "NO SD CARD";
    case music::State::Empty: return "NO MP3 FILES";
    case music::State::Stopped: return "STOPPED";
    case music::State::Playing: return "PLAYING";
    case music::State::Paused: return "PAUSED";
    case music::State::Error: return "PLAYER ERROR";
    }
    return "";
}
}

void appPlayer() {
    const auto player = music::snapshot();
    const bool ready = player.state != music::State::Starting;
    const uint16_t count = ready ? player.count : 0;
    listMenuItems = count + controlCount;
    if (listMenuNum >= listMenuItems) listMenuNum = listMenuItems - 1;

    if (isSubMenuButtonClicked && !isDisplayMessageWindow && ready) {
        bool queued;
        if (listMenuNum < count) {
            queued = music::command(listMenuNum == player.current &&
                (player.state == music::State::Playing || player.state == music::State::Paused)
                ? music::Action::TogglePause : music::Action::Play, listMenuNum);
        } else {
            static const music::Action actions[] = {
                music::Action::TogglePause, music::Action::Stop, music::Action::Previous,
                music::Action::Next, music::Action::VolumeDown, music::Action::VolumeUp,
                music::Action::BoostDown, music::Action::BoostUp,
                music::Action::Rescan, music::Action::Stop
            };
            // Last row returns encoder control to the submenu.
            if (listMenuNum == count + controlCount - 1) {
                isSubMenuEncoderControlList = false;
                queued = true;
            } else queued = music::command(actions[listMenuNum - count]);
        }
        if (!queued) Serial.println("Music command queue busy");
    }

    gfx->setTextColor(COLOR_MAIN);
    gfx->setCursor(10, 78);
    gfx->printf("%s V%u +%udB %u%s MP3", stateName(player.state), player.volume, player.boostDb,
                count, player.limited ? "+" : "");
    const uint16_t maxStart = listMenuItems > visibleRows ? listMenuItems - visibleRows : 0;
    const uint16_t start = std::min<uint16_t>(listMenuNum > 2 ? listMenuNum - 2 : 0, maxStart);
    static const char* controls[] = {"[PLAY / PAUSE]", "[STOP]", "[PREVIOUS]", "[NEXT]",
                                     "[VOLUME -]", "[VOLUME +]", "[BOOST -]", "[BOOST + / MAX 90dB]",
                                     "[RESCAN SD]", "[BACK]"};
    for (uint16_t i = start; i < listMenuItems && i < start + visibleRows; ++i) {
        const int y = 102 + (i - start) * CHAR_HEIGHT;
        const bool selected = i == listMenuNum;
        if (selected) gfx->fillRect(8, y - 18, 454, CHAR_HEIGHT, COLOR_MAIN);
        gfx->setTextColor(selected ? COLOR_SCREEN : COLOR_MAIN);
        gfx->setCursor(12, y);
        if (i < count) {
            const auto track = music::track(i);
            std::string label = (i == player.current ? "> " : "  ") + track.title;
            if (!track.artist.empty()) label += " - " + track.artist;
            gfx->print(displayText(label, 44));
        } else gfx->print(controls[i - count]);
    }
    gfx->setTextColor(COLOR_MAIN);
    gfx->setCursor(10, 216);
    const auto selected = listMenuNum < count ? music::track(listMenuNum) : player.nowPlaying;
    std::string details = selected.artist;
    if (!selected.album.empty()) details += (details.empty() ? "" : " / ") + selected.album;
    if (!player.error.empty()) details = player.error;
    else if (player.state == music::State::Empty) details = "Copy MP3 files to SD, then RESCAN";
    gfx->print(displayText(details, 45));
    gfx->setCursor(10, 239);
    gfx->print(displayText(player.nowPlaying.title.empty() ? "Select a track to play"
                                                        : "Now: " + player.nowPlaying.title, 45));
    gfx->setCursor(10, 263);
    gfx->printf("%lu:%02lu / %lu:%02lu  %s", static_cast<unsigned long>(player.elapsed / 60),
                static_cast<unsigned long>(player.elapsed % 60),
                static_cast<unsigned long>(player.duration / 60),
                static_cast<unsigned long>(player.duration % 60),
                isSubMenuEncoderControlList ? "HOLD: BACK" : "CLICK: BROWSE");
}
