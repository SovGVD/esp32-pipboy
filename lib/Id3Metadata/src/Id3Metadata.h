#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace id3 {
// Random access allows large artwork frames to be skipped without loading them.
class Reader {
public:
    virtual ~Reader() = default;
    virtual uint32_t size() const = 0;
    virtual bool read(uint32_t offset, uint8_t* data, size_t count) = 0;
};

struct Metadata {
    std::string title;
    std::string artist;
    std::string album;
};

// Reads ID3v2.2/2.3/2.4, filling missing fields from ID3v1. Never reads beyond
// the file or allocates according to untrusted tag/frame lengths.
Metadata read(Reader& file);
}
