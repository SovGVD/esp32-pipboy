#include "Id3Metadata.h"
#include <algorithm>
#include <cstring>

namespace id3 {
namespace {
constexpr size_t maxTextBytes = 768;
constexpr size_t maxOutputBytes = 192;

uint32_t bigEndian(const uint8_t* data, size_t count) {
    uint32_t value = 0;
    while (count--) value = (value << 8) | *data++;
    return value;
}

bool syncSafe(const uint8_t* data, uint32_t& value) {
    value = 0;
    for (size_t i = 0; i < 4; ++i) {
        if (data[i] & 0x80) return false;
        value = (value << 7) | data[i];
    }
    return true;
}

void appendUtf8(std::string& out, uint32_t cp) {
    if (cp < 32 || cp == 127) cp = ' ';
    if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) cp = '?';
    char bytes[4];
    size_t count;
    if (cp < 0x80) { bytes[0] = cp; count = 1; }
    else if (cp < 0x800) {
        bytes[0] = 0xc0 | (cp >> 6); bytes[1] = 0x80 | (cp & 63); count = 2;
    } else if (cp < 0x10000) {
        bytes[0] = 0xe0 | (cp >> 12); bytes[1] = 0x80 | ((cp >> 6) & 63);
        bytes[2] = 0x80 | (cp & 63); count = 3;
    } else {
        bytes[0] = 0xf0 | (cp >> 18); bytes[1] = 0x80 | ((cp >> 12) & 63);
        bytes[2] = 0x80 | ((cp >> 6) & 63); bytes[3] = 0x80 | (cp & 63); count = 4;
    }
    if (out.size() + count <= maxOutputBytes) out.append(bytes, count);
}

void trim(std::string& text) {
    const auto start = text.find_first_not_of(' ');
    if (start == std::string::npos) { text.clear(); return; }
    text = text.substr(start, text.find_last_not_of(' ') - start + 1);
}

std::string decode(const uint8_t* data, size_t size, uint8_t encoding) {
    std::string text;
    bool littleEndian = false;
    size_t i = 0;
    if (encoding == 1) {
        if (size < 2) return text;
        if (data[0] == 0xff && data[1] == 0xfe) littleEndian = true;
        else if (data[0] != 0xfe || data[1] != 0xff) return text;
        i = 2;
    }
    if (encoding > 3) return text;
    while (i < size && text.size() < maxOutputBytes) {
        uint32_t cp = data[i++];
        if (encoding == 1 || encoding == 2) {
            if (i >= size) break;
            cp = littleEndian ? cp | (data[i] << 8) : (cp << 8) | data[i];
            ++i;
            if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < size) {
                const uint32_t low = littleEndian ? data[i] | (data[i + 1] << 8)
                                                 : (data[i] << 8) | data[i + 1];
                if (low >= 0xdc00 && low <= 0xdfff) {
                    cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                    i += 2;
                }
            }
        } else if (encoding == 3 && cp >= 128) {
            const size_t extra = cp >= 0xf0 ? 3 : cp >= 0xe0 ? 2 : cp >= 0xc2 ? 1 : 0;
            if (!extra || i + extra > size || cp > 0xf4) break;
            cp &= (1U << (6 - extra)) - 1;
            bool valid = true;
            for (size_t n = 0; n < extra; ++n) {
                if ((data[i] & 0xc0) != 0x80) { valid = false; break; }
                cp = (cp << 6) | (data[i++] & 63);
            }
            if (!valid) break;
            if (cp < (extra == 1 ? 0x80U : extra == 2 ? 0x800U : 0x10000U)) cp = '?';
        }
        if (!cp) break; // first value of a multi-value text frame
        const size_t before = text.size();
        appendUtf8(text, cp);
        if (text.size() == before) break;
    }
    trim(text);
    return text;
}

std::string* field(Metadata& tags, const uint8_t* name, size_t length) {
    if (!std::memcmp(name, length == 3 ? "TT2" : "TIT2", length)) return &tags.title;
    if (!std::memcmp(name, length == 3 ? "TP1" : "TPE1", length)) return &tags.artist;
    if (!std::memcmp(name, length == 3 ? "TAL" : "TALB", length)) return &tags.album;
    return nullptr;
}

void readV2(Reader& file, Metadata& tags) {
    uint8_t header[10];
    if (file.size() < 10 || !file.read(0, header, 10) || std::memcmp(header, "ID3", 3)) return;
    const uint8_t version = header[3], flags = header[5];
    if (version < 2 || version > 4 || header[4] == 0xff) return;
    // v2.2 compression and v2.2/2.3 whole-tag unsynchronisation are unsupported.
    if ((version == 2 && (flags & 0x40)) || (version < 4 && (flags & 0x80))) return;
    uint32_t tagSize;
    if (!syncSafe(header + 6, tagSize) || tagSize > file.size() - 10) return;
    const uint32_t end = 10 + tagSize;
    uint32_t pos = 10;
    if (version >= 3 && (flags & 0x40)) {
        uint8_t ext[4];
        if (end - pos < 4 || !file.read(pos, ext, 4)) return;
        uint32_t skip;
        if (version == 4) {
            if (!syncSafe(ext, skip) || skip < 6) return;
        } else {
            skip = bigEndian(ext, 4);
            if (skip < 6 || skip > end - pos - 4) return;
            skip += 4; // v2.3 excludes its size field
        }
        if (skip > end - pos) return;
        pos += skip;
    }
    const size_t nameSize = version == 2 ? 3 : 4;
    const size_t headerSize = version == 2 ? 6 : 10;
    for (unsigned frame = 0; frame < 512 && end - pos >= headerSize; ++frame) {
        uint8_t h[10] = {};
        if (!file.read(pos, h, headerSize) || !h[0]) break;
        for (size_t c = 0; c < nameSize; ++c) {
            if (!((h[c] >= 'A' && h[c] <= 'Z') || (h[c] >= '0' && h[c] <= '9'))) return;
        }
        uint32_t size;
        if (version == 4) { if (!syncSafe(h + 4, size)) break; }
        else size = bigEndian(h + nameSize, version == 2 ? 3 : 4);
        pos += headerSize;
        if (size > end - pos) break;
        auto* target = field(tags, h, nameSize);
        const bool unsupported = (version == 3 && (h[9] & 0xe0)) ||
                                 (version == 4 && (h[9] & 0x4d));
        if (target && target->empty() && size && !unsupported) {
            uint8_t bytes[maxTextBytes];
            size_t count = std::min<size_t>(size, sizeof(bytes));
            if (!file.read(pos, bytes, count)) return;
            if (version == 4 && ((flags & 0x80) || (h[9] & 2))) {
                size_t out = 0;
                for (size_t i = 0; i < count; ++i) {
                    bytes[out++] = bytes[i];
                    if (bytes[i] == 0xff && i + 1 < count && bytes[i + 1] == 0) ++i;
                }
                count = out;
            }
            *target = decode(bytes + 1, count - 1, bytes[0]);
        }
        pos += size;
        if (!tags.title.empty() && !tags.artist.empty() && !tags.album.empty()) break;
    }
}
}

Metadata read(Reader& file) {
    Metadata tags;
    readV2(file, tags);
    uint8_t v1[128];
    if (file.size() >= sizeof(v1) && file.read(file.size() - sizeof(v1), v1, sizeof(v1)) &&
        !std::memcmp(v1, "TAG", 3)) {
        if (tags.title.empty()) tags.title = decode(v1 + 3, 30, 0);
        if (tags.artist.empty()) tags.artist = decode(v1 + 33, 30, 0);
        if (tags.album.empty()) tags.album = decode(v1 + 63, 30, 0);
    }
    return tags;
}
}
