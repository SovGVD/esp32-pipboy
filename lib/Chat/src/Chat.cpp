#include "Chat.h"
#include <cstring>

namespace chat {
namespace {
const char lowerKeys[] = "1234567890" "qwertyuiop" "asdfghjkl'" "zxcvbnm,.?";
const char upperKeys[] = "!@#$%^&*()" "QWERTYUIOP" "ASDFGHJKL\"" "ZXCVBNM;:-";
static_assert(sizeof(lowerKeys) == characterKeys + 1, "Keyboard rows must be full");
static_assert(sizeof(upperKeys) == characterKeys + 1, "Keyboard rows must be full");

void copyText(char* out, size_t outSize, const char* in) {
    std::strncpy(out, in ? in : "", outSize - 1);
    out[outSize - 1] = '\0';
}
}

size_t sanitize(char* out, size_t outSize, const char* in) {
    if (!out || !outSize) return 0;
    size_t length = 0;
    bool space = false;
    for (const char* p = in; p && *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 0x20 || c == 0x7f) c = ' ';
        else if (c > 0x7e) c = '?';
        if (c == ' ') { space = length > 0; continue; }
        if (length + (space ? 2 : 1) > outSize - 1) break;
        if (space) out[length++] = ' ';
        space = false;
        out[length++] = static_cast<char>(c);
    }
    out[length] = '\0';
    return length;
}

size_t wrap(const char* text, unsigned width, Line* lines, size_t maxLines) {
    if (!text || !width) return 0;
    const size_t length = std::strlen(text);
    size_t count = 0, position = 0;
    while (position < length && position <= 255) {
        while (position < length && text[position] == ' ') ++position;
        if (position >= length) break;
        size_t take = length - position;
        if (take > width) {
            // Break after the last space that keeps the line within width.
            size_t split = position + width;
            while (split > position && text[split] != ' ') --split;
            take = split > position ? split - position : width;
        }
        size_t visible = take;
        while (visible && text[position + visible - 1] == ' ') --visible;
        if (count < maxLines) {
            lines[count].start = static_cast<uint8_t>(position);
            lines[count].length = static_cast<uint8_t>(visible);
        }
        ++count;
        position += take;
    }
    return count;
}

History::History() : messages_(), start_(0), size_(0) {}

void History::add(const char* peer, const char* text, bool outgoing, bool broadcast) {
    Message& message = messages_[(start_ + size_) % historySize];
    if (size_ < historySize) ++size_;
    else start_ = (start_ + 1) % historySize;
    copyText(message.peer, sizeof(message.peer), peer);
    sanitize(message.text, sizeof(message.text), text);
    message.outgoing = outgoing;
    message.broadcast = broadcast;
}

const Message& History::at(size_t index) const {
    return messages_[(start_ + (index < size_ ? index : size_ - 1)) % historySize];
}

bool inConversation(const Message& message, const char* peer, bool broadcast) {
    if (broadcast || message.broadcast) return broadcast == message.broadcast;
    return peer && std::strcmp(message.peer, peer) == 0;
}

Key keyAt(unsigned index) {
    if (index < characterKeys) return Key::Character;
    switch (index - characterKeys) {
        case 0: return Key::Shift;
        case 1: return Key::Space;
        case 2: return Key::Delete;
        default: return Key::Send;
    }
}

char characterAt(unsigned index, bool shift) {
    if (index >= characterKeys) return '\0';
    return shift ? upperKeys[index] : lowerKeys[index];
}

Composer::Composer() : selection_(firstKey), shift_(false), draft_(), length_(0) {}

void Composer::turn(int detents) {
    const int count = static_cast<int>(keyCount);
    int selection = (static_cast<int>(selection_) + detents % count) % count;
    if (selection < 0) selection += count;
    selection_ = static_cast<unsigned>(selection);
}

bool Composer::press(char (&out)[maxText + 1]) {
    switch (keyAt(selection_)) {
        case Key::Character:
            if (length_ < maxText) draft_[length_++] = characterAt(selection_, shift_);
            break;
        case Key::Shift:
            shift_ = !shift_;
            break;
        case Key::Space:
            if (length_ && length_ < maxText && draft_[length_ - 1] != ' ') draft_[length_++] = ' ';
            break;
        case Key::Delete:
            if (length_) --length_;
            break;
        case Key::Send:
            if (!sanitize(out, sizeof(out), draft_)) return false;
            clear();
            return true;
    }
    draft_[length_] = '\0';
    return false;
}

void Composer::clear() {
    length_ = 0;
    draft_[0] = '\0';
}
}
