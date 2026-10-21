#include "Chat.h"
#include <cstdio>
#include <cstring>

namespace chat {
namespace {
constexpr uint16_t ink = 0x45a2, dim = 0x2201, black = 0;
constexpr int left = 12, right = 467, charWidth = 10;
constexpr int historyBaseline = 76, historyPitch = 18;
constexpr int inputTop = 120, inputHeight = 22;
constexpr int keysX = 15, keysTop = 147, keyPitchX = 45, keyPitchY = 23, keyHeight = 21;
constexpr size_t visibleDraft = 35;

void box(Canvas& canvas, int x, int y, int width, int height, uint16_t color) {
    canvas.line(x,y,x+width-1,y,color); canvas.line(x,y+height-1,x+width-1,y+height-1,color);
    canvas.line(x,y,x,y+height-1,color); canvas.line(x+width-1,y,x+width-1,y+height-1,color);
}

void header(Canvas& canvas, const View& view) {
    char text[nameSize + 16];
    std::snprintf(text, sizeof(text), "TO: %s", view.broadcast ? "ALL" : view.peer);
    canvas.text(left, 51, text, ink);
    if (!view.broadcast)
        canvas.text(left + (static_cast<int>(std::strlen(text)) + 1) * charWidth, 51,
                    view.online ? "ONLINE" : "OFFLINE", view.online ? ink : dim);
}

void history(Canvas& canvas, const View& view) {
    struct Shown { const char* text; Line line; };
    char formatted[historyLines][nameSize + 2 + maxText + 16 + 1];
    Shown shown[historyLines];
    unsigned count = 0, used = 0;
    for (size_t i = view.history ? view.history->size() : 0; i-- > 0 && count < historyLines;) {
        const Message& message = view.history->at(i);
        if (!inConversation(message, view.peer, view.broadcast)) continue;
        char* text = formatted[used++];
        const char* tag = message.delivery == Delivery::Pending ? " (...)" :
            message.delivery == Delivery::Failed ? " (NOT DELIVERED)" : "";
        std::snprintf(text, sizeof(formatted[0]), "%s: %s%s", message.outgoing ? "ME" : message.peer, message.text, tag);
        Line lines[4];
        size_t total = wrap(text, lineWidth, lines, 4);
        if (total > 4) total = 4;
        for (size_t l = total; l-- > 0 && count < historyLines;) shown[count++] = {text, lines[l]};
    }
    if (!count) {
        canvas.text(left + 3, historyBaseline, "NO MESSAGES YET", dim);
        return;
    }
    for (unsigned row = 0; row < count; ++row) {
        const Shown& item = shown[count - 1 - row];
        char text[lineWidth + 1];
        std::memcpy(text, item.text + item.line.start, item.line.length);
        text[item.line.length] = '\0';
        canvas.text(left + 3, historyBaseline + row * historyPitch, text, ink);
    }
}

void input(Canvas& canvas, const View& view) {
    box(canvas, left, inputTop, right - left + 1, inputHeight, view.typing ? ink : dim);
    const Composer& composer = *view.composer;
    const size_t length = composer.length();
    const char* tail = composer.draft() + (length > visibleDraft ? length - visibleDraft : 0);
    char text[visibleDraft + 4];
    std::snprintf(text, sizeof(text), ">%s%s", tail, view.typing ? "_" : "");
    canvas.text(left + 5, inputTop + 16, text, ink);
    char counter[8];
    std::snprintf(counter, sizeof(counter), "%u/%u", static_cast<unsigned>(length), static_cast<unsigned>(maxText));
    canvas.text(right - 4 - static_cast<int>(std::strlen(counter)) * charWidth, inputTop + 16, counter, dim);
}

void key(Canvas& canvas, int column, int row, int units, const char* label, bool selected, bool highlight) {
    const int x = keysX + column * keyPitchX, y = keysTop + row * keyPitchY;
    const int width = units * keyPitchX - 2;
    if (selected) canvas.rectangle(x, y, width, keyHeight, ink);
    else box(canvas, x, y, width, keyHeight, highlight ? ink : dim);
    const int textX = x + (width - static_cast<int>(std::strlen(label)) * charWidth) / 2;
    canvas.text(textX, y + 16, label, selected ? black : ink);
}

void keyboard(Canvas& canvas, const View& view) {
    const Composer& composer = *view.composer;
    const unsigned selection = view.typing ? composer.selection() : keyCount;
    for (unsigned index = 0; index < characterKeys; ++index) {
        const char label[] = {characterAt(index, composer.shift()), '\0'};
        key(canvas, index % keyColumns, index / keyColumns, 1, label, index == selection, false);
    }
    struct Special { unsigned column, units; const char* label; };
    const Special specials[] = {{0,2,composer.shift() ? "[SHIFT]" : "SHIFT"}, {2,4,"SPACE"}, {6,2,"DEL"}, {8,2,"SEND"}};
    for (unsigned i = 0; i < 4; ++i)
        key(canvas, specials[i].column, characterRows, specials[i].units, specials[i].label,
            characterKeys + i == selection, i == 0 && composer.shift());
}
}

unsigned keyIndexAt(int x, int y) {
    if (x < keysX || y < keysTop) return keyCount;
    const int column = (x - keysX) / keyPitchX, row = (y - keysTop) / keyPitchY;
    // Taps in the 2-pixel gaps count for the key to their left or above.
    if (column >= static_cast<int>(keyColumns) || row > static_cast<int>(characterRows)) return keyCount;
    if (row < static_cast<int>(characterRows)) return row * keyColumns + column;
    const unsigned special = column < 2 ? 0 : column < 6 ? 1 : column < 8 ? 2 : 3;
    return characterKeys + special;
}

void render(Canvas& canvas, const View& view) {
    header(canvas, view);
    history(canvas, view);
    if (!view.composer) return;
    input(canvas, view);
    keyboard(canvas, view);
}
}
