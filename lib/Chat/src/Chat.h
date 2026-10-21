#pragma once

#include <cstddef>
#include <cstdint>

namespace chat {
// 80 characters still fit a 126-byte ESP-NOW frame with two 15-character names.
constexpr size_t maxText = 80;
constexpr size_t nameSize = 16;
constexpr size_t historySize = 24;
constexpr unsigned keyColumns = 10, characterRows = 4;
constexpr unsigned characterKeys = keyColumns * characterRows;
constexpr unsigned keyCount = characterKeys + 4;
constexpr unsigned firstKey = keyColumns; // 'q'
enum class Key : uint8_t { Character, Shift, Space, Delete, Send };

struct Message {
    char peer[nameSize]; // sender of incoming, recipient of outgoing messages
    char text[maxText + 1];
    bool outgoing, broadcast;
};

// Copies printable ASCII only. Tabs/control bytes become spaces, other bytes
// become '?', runs of spaces collapse, both ends are trimmed and the result is
// truncated to outSize-1. Returns the copied length; out is always terminated.
size_t sanitize(char* out, size_t outSize, const char* in);

struct Line { uint8_t start, length; };
// Word-wraps text into lines of at most width characters, breaking long words.
// Returns the number of lines, writing at most maxLines of them.
size_t wrap(const char* text, unsigned width, Line* lines, size_t maxLines);

// Fixed ring buffer of the most recent messages across all conversations.
class History {
public:
    History();
    void add(const char* peer, const char* text, bool outgoing, bool broadcast);
    size_t size() const { return size_; }
    const Message& at(size_t index) const; // 0 is the oldest kept message
    void clear() { start_ = size_ = 0; }
private:
    Message messages_[historySize];
    size_t start_, size_;
};
// Broadcast messages make up the ALL conversation; others match the peer name.
bool inConversation(const Message& message, const char* peer, bool broadcast);

Key keyAt(unsigned index);
char characterAt(unsigned index, bool shift);

// On-screen QWERTY keyboard and draft. The single encoder walks all keys in
// reading order and wraps around, so SEND is one detent left of 'q's row start.
class Composer {
public:
    Composer();
    unsigned selection() const { return selection_; }
    bool shift() const { return shift_; }
    const char* draft() const { return draft_; }
    size_t length() const { return length_; }
    void turn(int detents);
    // Applies the selected key. Returns true when SEND had text to send: the
    // sanitized message is copied to out and the draft is cleared.
    bool press(char (&out)[maxText + 1]);
    void clear();
private:
    unsigned selection_;
    bool shift_;
    char draft_[maxText + 1];
    size_t length_;
};

class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void line(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void rectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void text(int x, int baseline, const char* text, uint16_t color) = 0;
};

struct View {
    const History* history;
    const Composer* composer;
    const char* peer;
    bool broadcast, online, typing;
};
constexpr unsigned historyLines = 3;
constexpr unsigned lineWidth = 45;
// Draws the conversation, draft and keyboard below the RADIO submenu.
void render(Canvas& canvas, const View& view);
}
