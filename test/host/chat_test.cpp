#include <Chat.h>
#include "NativeFrame.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

// NativeFrame rasterizes the arcade's triangles too; chat never draws them.
struct TriangleCanvas : chat::Canvas {
    virtual void triangle(int, int, int, int, int, int, uint16_t) = 0;
};
using Frame = NativeFrame<TriangleCanvas>;

namespace {
// Moves the selection to a key, the way a user turns the encoder.
void select(chat::Composer& composer, unsigned index) {
    composer.turn(static_cast<int>(index) - static_cast<int>(composer.selection()));
    assert(composer.selection() == index);
}
unsigned indexOf(char character, bool& shift) {
    for (unsigned i = 0; i < chat::characterKeys; ++i) {
        if (chat::characterAt(i, false) == character) { shift = false; return i; }
        if (chat::characterAt(i, true) == character) { shift = true; return i; }
    }
    assert(!"character is not on the keyboard");
    return 0;
}
constexpr unsigned shiftKey = chat::characterKeys, spaceKey = shiftKey + 1, deleteKey = shiftKey + 2, sendKey = shiftKey + 3;
// Types text with the keyboard; returns true and fills out if it ends with SEND.
bool type(chat::Composer& composer, const char* text, char (&out)[chat::maxText + 1], bool send = true) {
    for (const char* c = text; *c; ++c) {
        if (*c == ' ') { select(composer, spaceKey); assert(!composer.press(out)); continue; }
        bool shift = false;
        const unsigned index = indexOf(*c, shift);
        if (shift != composer.shift()) { select(composer, shiftKey); assert(!composer.press(out)); }
        select(composer, index);
        assert(!composer.press(out));
    }
    if (!send) return false;
    select(composer, sendKey);
    return composer.press(out);
}

void header(Frame& frame) {
    constexpr uint16_t ink = 0x45a2, dim = 0x2201;
    frame.font = &monofonto11pt7b;
    const char* names[] = {"STAT","INV","DATA","MAP","RADIO"};
    const int positions[] = {60,140,210,290,360};
    for (unsigned i = 0; i < 5; ++i) frame.fixedText(positions[i],21,names[i],ink);
    frame.line(0,27,353,27,ink); frame.line(353,27,353,11,ink);
    frame.line(353,11,357,11,ink); frame.line(413,11,417,11,ink);
    frame.line(417,11,417,27,ink); frame.line(417,27,470,27,ink);
    frame.fixedText(299,51,"MUSIC",dim); frame.fixedText(360,51,"CHAT",ink);
    frame.font = &monofonto10pt7b;
}
void save(Frame& frame, int argc, char** argv, const char* name) {
    if (argc == 2) frame.save(std::string(argv[1]) + "/" + name + ".ppm");
}
}

int main(int argc, char** argv) {
    char out[chat::maxText + 1];

    // Sanitizing: printable ASCII, collapsed/trimmed spaces, safe truncation.
    assert(chat::sanitize(out, sizeof(out), "  hi\tthere\n\x01 ok  ") == 11 && !std::strcmp(out, "hi there ok"));
    assert(chat::sanitize(out, sizeof(out), "caf\xc3\xa9") == 5 && !std::strcmp(out, "caf??"));
    assert(chat::sanitize(out, sizeof(out), "   ") == 0 && !out[0]);
    assert(chat::sanitize(out, sizeof(out), nullptr) == 0 && !out[0]);
    char tiny[4];
    assert(chat::sanitize(tiny, sizeof(tiny), "ab cd") == 2 && !std::strcmp(tiny, "ab"));
    assert(chat::sanitize(tiny, sizeof(tiny), "abcdef") == 3 && !std::strcmp(tiny, "abc"));
    const std::string longText(200, 'x');
    assert(chat::sanitize(out, sizeof(out), longText.c_str()) == chat::maxText);

    // Word wrap never exceeds the width, even for a word longer than a line.
    chat::Line lines[8];
    assert(chat::wrap("hello world", 20, lines, 8) == 1 && lines[0].length == 11);
    assert(chat::wrap("hello world", 5, lines, 8) == 2 && lines[1].start == 6 && lines[1].length == 5);
    assert(chat::wrap("abcdefghijkl", 5, lines, 8) == 3 && lines[2].length == 2);
    assert(chat::wrap("aa bbbbbbbbbbbb", 5, lines, 8) == 4 && lines[0].length == 2 && lines[1].length == 5);
    assert(chat::wrap("", 5, lines, 8) == 0 && chat::wrap("x", 0, lines, 8) == 0);
    assert(chat::wrap(longText.c_str(), 45, lines, 2) == 5); // counts lines beyond the output array

    // The keyboard is QWERTY with digits on top and wraps in both directions.
    assert(chat::characterAt(10, false) == 'q' && chat::characterAt(19, false) == 'p');
    assert(chat::characterAt(20, true) == 'A' && chat::characterAt(30, false) == 'z');
    for (unsigned i = 0; i < chat::characterKeys; ++i) {
        for (bool shift : {false, true}) {
            const char c = chat::characterAt(i, shift);
            assert(c > ' ' && c <= '~');
        }
    }
    chat::Composer composer;
    assert(composer.selection() == chat::firstKey && !composer.length());
    composer.turn(-chat::firstKey - 1); assert(composer.selection() == sendKey);
    composer.turn(1); assert(composer.selection() == 0);
    composer.turn(static_cast<int>(chat::keyCount) * 3 + 2); assert(composer.selection() == 2);
    composer.turn(-100); composer.turn(100); assert(composer.selection() == 2);
    select(composer, sendKey); assert(!composer.press(out)); // empty drafts are not sent

    assert(type(composer, "Hello, Vault 111!", out) && !std::strcmp(out, "Hello, Vault 111!"));
    assert(!composer.length() && !composer.draft()[0]);
    // Leading/double spaces are not typed; DEL removes the last character.
    type(composer, " a  b", out, false); assert(!std::strcmp(composer.draft(), "a b"));
    select(composer, deleteKey); composer.press(out); composer.press(out); assert(!std::strcmp(composer.draft(), "a"));
    composer.press(out); composer.press(out); assert(!composer.length());
    // Drafts stop at the message limit.
    if (composer.shift()) { select(composer, shiftKey); composer.press(out); }
    select(composer, 11);
    for (unsigned i = 0; i < 200; ++i) composer.press(out);
    assert(composer.length() == chat::maxText);
    select(composer, sendKey); assert(composer.press(out) && std::strlen(out) == chat::maxText);

    // History keeps the newest messages and splits them into conversations.
    chat::History history;
    for (unsigned i = 0; i < chat::historySize + 5; ++i) history.add("BOB", std::to_string(i).c_str(), i % 2, false);
    assert(history.size() == chat::historySize && !std::strcmp(history.at(0).text, "5"));
    assert(!std::strcmp(history.at(chat::historySize - 1).text, std::to_string(chat::historySize + 4).c_str()));
    history.clear();
    history.add("BOB", "hey", false, false);
    history.add("BOB", "everyone", false, true);
    history.add("ALICE", "hi\tthere", true, false);
    assert(!std::strcmp(history.at(2).text, "hi there"));
    assert(chat::inConversation(history.at(0), "BOB", false) && !chat::inConversation(history.at(0), "ALICE", false));
    assert(!chat::inConversation(history.at(1), "BOB", false) && chat::inConversation(history.at(1), "_", true));
    assert(!chat::inConversation(history.at(0), "_", true));

    // Packet bodies carry a 4-digit hex id before the text.
    char body[chat::maxText + 8];
    assert(chat::formatBody(body, sizeof(body), 0xBEEF, "hi there") == 13 && !std::strcmp(body, "BEEF hi there"));
    const std::string maxBody = std::string(chat::maxText, 'w');
    assert(chat::formatBody(body, sizeof(body), 7, maxBody.c_str()) == chat::maxText + 5);
    assert(chat::formatBody(body, 6, 7, "hi") == 0);
    uint16_t id = 0;
    const char* text = nullptr;
    assert(chat::parseBody("00a1 hello", id, text) && id == 0xa1 && !std::strcmp(text, "hello"));
    assert(chat::parseBody("FFFF ", id, text) && id == 0xffff && !*text);
    assert(!chat::parseBody("FFFF", id, text) && !chat::parseBody("FFF hello", id, text));
    assert(!chat::parseBody("FFFFhello", id, text) && !chat::parseBody("XYZW hi", id, text) && !chat::parseBody(nullptr, id, text));
    assert(chat::formatId(body, sizeof(body), 0x12) == 4 && !std::strcmp(body, "0012"));
    assert(chat::parseId("0012", id) && id == 0x12 && !chat::parseId("0012x", id) && !chat::parseId("12", id));
    // A 126-byte frame fits the longest body with two 15-character names.
    assert(std::strlen("PIPBOY\t") + 2 * 16 + std::strlen("M\t") + chat::maxText + 5 <= 126);

    // Direct messages retry until acknowledged; broadcasts never wait.
    chat::History delivery;
    chat::Message& direct = delivery.add("BOB", "ping", true, false, 41, 1000);
    assert(direct.delivery == chat::Delivery::Pending && direct.attempts == 1);
    assert(delivery.add("_", "all", true, true, 42, 1000).delivery == chat::Delivery::None);
    assert(delivery.add("BOB", "in", false, false, 41, 1000).delivery == chat::Delivery::None);
    assert(!delivery.dueForRetry(1000 + chat::retryIntervalMs - 1));
    assert(delivery.dueForRetry(1000 + chat::retryIntervalMs) == &direct && direct.attempts == 2);
    assert(!delivery.dueForRetry(1000 + chat::retryIntervalMs));
    assert(!delivery.acknowledge("ALICE", 41) && !delivery.acknowledge("BOB", 40));
    assert(delivery.acknowledge("BOB", 41) && direct.delivery == chat::Delivery::Delivered);
    assert(!delivery.dueForRetry(100000));
    // Without an acknowledgement it fails after the last attempt's interval.
    chat::Message& lost = delivery.add("BOB", "lost", true, false, 43, 0xFFFFFF00u); // across the millis() wrap
    uint32_t now = 0xFFFFFF00u;
    for (unsigned attempt = 2; attempt <= chat::sendAttempts; ++attempt) {
        now += chat::retryIntervalMs;
        assert(delivery.dueForRetry(now) == &lost && lost.attempts == attempt);
    }
    assert(!delivery.dueForRetry(now + chat::retryIntervalMs) && lost.delivery == chat::Delivery::Failed);
    assert(delivery.acknowledge("BOB", 43) && lost.delivery == chat::Delivery::Delivered); // late ACK still counts

    // Resent copies are recognized per sender.
    chat::RecentIds recent;
    assert(!recent.seen("BOB", 1) && recent.seen("BOB", 1) && !recent.seen("ALICE", 1));
    for (uint16_t i = 100; i < 120; ++i) assert(!recent.seen("BOB", i));
    assert(!recent.seen("BOB", 1)); // forgotten after 16 newer ids

    // Touch hits the key that is drawn there, including the wide bottom row.
    for (unsigned key = 0; key < chat::characterKeys; ++key)
        assert(chat::keyIndexAt(15 + (key % 10) * 45 + 21, 147 + (key / 10) * 23 + 10) == key);
    assert(chat::keyIndexAt(15, 147) == 0 && chat::keyIndexAt(464, 146 + 4 * 23) == 39);
    assert(chat::keyIndexAt(100, 245) == shiftKey && chat::keyIndexAt(200, 245) == spaceKey);
    assert(chat::keyIndexAt(330, 245) == deleteKey && chat::keyIndexAt(440, 259) == sendKey);
    assert(chat::keyIndexAt(14, 200) == chat::keyCount && chat::keyIndexAt(100, 146) == chat::keyCount);
    assert(chat::keyIndexAt(466, 200) == chat::keyCount && chat::keyIndexAt(100, 263) == chat::keyCount);
    assert(chat::keyIndexAt(-5, -5) == chat::keyCount);
    chat::Composer tapped;
    tapped.select(chat::keyIndexAt(200, 245)); assert(tapped.selection() == spaceKey);
    tapped.select(chat::keyCount); assert(tapped.selection() == spaceKey);

    // Every screen state renders inside the native 480x272 frame.
    chat::History demo;
    demo.add("DOGMEAT", "Anyone near Diamond City? Need stimpaks.", false, false);
    demo.add("DOGMEAT", "On my way, bringing water", true, false, 1).delivery = chat::Delivery::Delivered;
    demo.add("DOGMEAT", "Great, meet at the gate", false, false);
    demo.add("DOGMEAT", "OK", true, false, 2);
    demo.add("PIPBOY042", "Radstorm coming from the east, take cover!", false, true);
    chat::Composer typing;
    type(typing, "See you in 5 min", out, false);
    select(typing, 25);
    chat::Composer shifted;
    select(shifted, shiftKey); shifted.press(out); select(shifted, 23);
    struct Case { const char* name; const chat::History* history; const chat::Composer* composer; const char* peer; bool broadcast, online, typing; };
    chat::History empty;
    chat::Composer full;
    select(full, 34);
    for (unsigned i = 0; i < 200; ++i) full.press(out);
    chat::History longest;
    longest.add("ABCDEFGHIJKLMNO", longText.c_str(), false, false);
    longest.add("ABCDEFGHIJKLMNO", longText.c_str(), true, false).delivery = chat::Delivery::Failed;
    const Case cases[] = {
        {"chat-typing", &demo, &typing, "DOGMEAT", false, true, true},
        {"chat-shift", &demo, &shifted, "DOGMEAT", false, false, true},
        {"chat-all", &demo, &composer, "_", true, true, true},
        {"chat-empty", &empty, &composer, "DOGMEAT", false, true, false},
        {"chat-full", &longest, &full, "ABCDEFGHIJKLMNO", false, false, true},
    };
    for (const Case& c : cases) {
        Frame frame; header(frame);
        const chat::View view = {c.history, c.composer, c.peer, c.broadcast, c.online, c.typing};
        chat::render(frame, view);
        assert(frame.texts > chat::keyCount);
        save(frame, argc, argv, c.name);
    }
    for (unsigned selection = 0; selection < chat::keyCount; ++selection) {
        chat::Composer each; select(each, selection);
        Frame frame;
        const chat::View view = {&demo, &each, "DOGMEAT", false, true, true};
        chat::render(frame, view);
    }
    std::cout << "Chat: sanitizing, word wrap, QWERTY keyboard, drafts, packet bodies, delivery retries, duplicate ids, history, touch hit testing and native bounds passed\n";
}
