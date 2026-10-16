#include <Id3Metadata.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

using Bytes = std::vector<uint8_t>;
struct MemoryReader : id3::Reader {
    Bytes bytes;
    unsigned calls = 0;
    explicit MemoryReader(Bytes value) : bytes(std::move(value)) {}
    uint32_t size() const override { return bytes.size(); }
    bool read(uint32_t offset, uint8_t* data, size_t count) override {
        ++calls;
        assert(offset <= bytes.size() && count <= bytes.size() - offset);
        std::memcpy(data, bytes.data() + offset, count);
        return true;
    }
};
Bytes number(uint32_t n, unsigned count, bool sync = false) {
    Bytes bytes(count);
    for (unsigned i = count; i-- > 0;) { bytes[i] = n & (sync ? 127 : 255); n >>= sync ? 7 : 8; }
    return bytes;
}
void append(Bytes& to, const Bytes& from) { to.insert(to.end(), from.begin(), from.end()); }
Bytes frame(unsigned version, const char* id, Bytes payload, uint8_t flags = 0) {
    Bytes bytes(id, id + (version == 2 ? 3 : 4));
    append(bytes, number(payload.size(), version == 2 ? 3 : 4, version == 4));
    if (version != 2) { bytes.push_back(0); bytes.push_back(flags); }
    append(bytes, payload);
    return bytes;
}
Bytes text(const std::string& value, uint8_t encoding = 3) {
    Bytes bytes{encoding};
    bytes.insert(bytes.end(), value.begin(), value.end());
    return bytes;
}
Bytes tag(unsigned version, Bytes frames, uint8_t flags = 0) {
    Bytes bytes{'I', 'D', '3', static_cast<uint8_t>(version), 0, flags};
    append(bytes, number(frames.size(), 4, true));
    append(bytes, frames);
    return bytes;
}
Bytes v1(const char* title, const char* artist, const char* album) {
    Bytes bytes(128, 0);
    std::memcpy(bytes.data(), "TAG", 3);
    std::memcpy(bytes.data() + 3, title, std::min<size_t>(30, std::strlen(title)));
    std::memcpy(bytes.data() + 33, artist, std::min<size_t>(30, std::strlen(artist)));
    std::memcpy(bytes.data() + 63, album, std::min<size_t>(30, std::strlen(album)));
    return bytes;
}
id3::Metadata parse(Bytes bytes) { MemoryReader reader(std::move(bytes)); return id3::read(reader); }

int main() {
    for (unsigned version : {2U, 3U, 4U}) {
        Bytes frames = frame(version, version == 2 ? "TT2" : "TIT2", text("Song", 0));
        append(frames, frame(version, version == 2 ? "TP1" : "TPE1", text("Artist", 0)));
        append(frames, frame(version, version == 2 ? "TAL" : "TALB", text("Album", 0)));
        const auto result = parse(tag(version, frames));
        assert(result.title == "Song" && result.artist == "Artist" && result.album == "Album");
    }
    assert(parse(v1(" Song  ", "Artist", "Album")).title == "Song");
    Bytes partial = tag(3, frame(3, "TIT2", text("Modern")));
    append(partial, v1("Old", "Legacy artist", "Legacy album"));
    const auto merged = parse(partial);
    assert(merged.title == "Modern" && merged.artist == "Legacy artist" && merged.album == "Legacy album");
    assert(parse(tag(4, frame(4, "TIT2", text("Caf\xc3\xa9")))).title == "Caf\xc3\xa9");
    assert(parse(tag(3, frame(3, "TIT2", text("Caf\xe9", 0)))).title == "Caf\xc3\xa9");
    assert(parse(tag(3, frame(3, "TIT2", {1, 0xff, 0xfe, 'H', 0, 'i', 0, 0, 0}))).title == "Hi");
    assert(parse(tag(3, frame(3, "TIT2", {1, 0xfe, 0xff, 0, 'H', 0, 'i'}))).title == "Hi");
    assert(parse(tag(4, frame(4, "TIT2", {2, 0, 'H', 0, 'i'}))).title == "Hi");
    assert(parse(tag(4, frame(4, "TIT2", {1, 0xff, 0xfe, 0x3d, 0xd8, 0, 0xde}))).title == "\xf0\x9f\x98\x80");
    assert(parse(tag(4, frame(4, "TIT2", {3, 'A', 0, 'B'}))).title == "A");
    assert(parse(tag(3, frame(3, "TIT2", {3, 'A', '\n', 'B'}))).title == "A B");
    assert(parse(tag(4, frame(4, "TIT2", {1, 'A', 0}))).title.empty()); // missing BOM
    assert(parse(tag(4, frame(4, "TIT2", {3, 0xe0, 0x80, 0x80}))).title == "?");
    for (unsigned version : {3U, 4U}) {
        Bytes ext = number(version == 3 ? 6 : 6, 4, version == 4);
        ext.resize(version == 3 ? 10 : 6, 0);
        append(ext, frame(version, "TIT2", text("Extended")));
        assert(parse(tag(version, ext, 0x40)).title == "Extended");
    }
    for (unsigned version : {2U, 3U, 4U}) {
        Bytes art = frame(version, version == 2 ? "PIC" : "APIC", Bytes(200000, 42));
        append(art, frame(version, version == 2 ? "TT2" : "TIT2", text("After artwork")));
        MemoryReader artwork(tag(version, art));
        assert(id3::read(artwork).title == "After artwork" && artwork.calls < 10);
    }
    assert(parse(tag(4, frame(4, "TIT2", text(std::string(5000, 'x'))))).title.size() == 192);
    assert(parse(tag(4, frame(4, "TIT2", {0, 'A', 0xff, 0, 0xe0}, 2))).title == "A\xc3\xbf\xc3\xa0");
    assert(parse(tag(3, frame(3, "TIT2", text("Compressed"), 0x80))).title.empty());
    assert(parse(tag(4, frame(4, "TIT2", text("Encrypted"), 4))).title.empty());
    assert(parse(tag(3, frame(3, "TIT2", text("Unsync")), 0x80)).title.empty());
    Bytes bad = tag(4, frame(4, "TIT2", text("Broken")));
    bad[6] = 0xff;
    assert(parse(bad).title.empty());
    bad = tag(3, frame(3, "TIT2", text("Broken")));
    bad[14] = 0x7f; // frame extends beyond tag
    assert(parse(bad).title.empty());
    Bytes hugeExt = {0x7f, 0x7f, 0x7f, 0x7f};
    assert(parse(tag(4, hugeExt, 0x40)).title.empty());
    const Bytes seed = tag(4, frame(4, "TIT2", text("Mutation test")));
    for (size_t n = 0; n < seed.size(); ++n) {
        Bytes truncated(seed.begin(), seed.begin() + n);
        assert(parse(truncated).title.empty());
    }
    std::mt19937 random(42);
    for (unsigned test = 0; test < 10000; ++test) {
        Bytes mutated = seed;
        for (unsigned n = 0; n < 1 + test % 6; ++n) mutated[random() % mutated.size()] = random();
        MemoryReader reader(std::move(mutated));
        const auto result = id3::read(reader);
        assert(reader.calls < 520 && result.title.size() <= 192);
    }
    std::cout << "ID3 version, encoding, bounds, artwork, fallback and mutation checks passed\n";
}
