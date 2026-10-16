#include <AudioBoost.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>

uint32_t pack(int32_t left, int32_t right) {
    return (static_cast<uint32_t>(left) & 0xffff) << 16 | (static_cast<uint32_t>(right) & 0xffff);
}
int32_t unpack(uint32_t bits) {
    bits &= 0xffff;
    return bits >= 0x8000 ? static_cast<int32_t>(bits) - 65536 : bits;
}

int main() {
    audio::Boost boost;
    std::mt19937 random(42);
    for (unsigned i = 0; i < 10000; ++i) {
        const uint32_t input = random();
        assert(boost.process(input) == input); // bypass is bit-exact
    }
    for (uint8_t level = 1; level <= audio::Boost::maxLevel; ++level) {
        boost.setLevel(level);
        boost.reset();
        assert(boost.decibels() == level * 6);
        const double expected = std::min<double>(audio::Boost::ceiling, 256 * std::pow(10, level * 6 / 20.0));
        const auto quiet = boost.process(pack(256, -128));
        assert(std::abs(unpack(quiet >> 16) - expected) < 2);
        assert(std::abs(unpack(quiet >> 16) + 2 * unpack(quiet)) <= 1);
        // Immediate transient limiting, both channels sharing the same gain.
        const auto loud = boost.process(pack(-32768, 16384));
        assert(unpack(loud >> 16) < 0 && unpack(loud) > 0);
        assert(std::abs(unpack(loud >> 16)) <= audio::Boost::ceiling);
        assert(std::abs(unpack(loud >> 16) + 2 * unpack(loud)) <= 1);
        for (unsigned i = 0; i < 10000; ++i) {
            const auto out = boost.process(random());
            assert(std::abs(unpack(out >> 16)) <= audio::Boost::ceiling);
            assert(std::abs(unpack(out)) <= audio::Boost::ceiling);
        }
        for (unsigned rate : {8000U, 44100U, 48000U, 96000U}) {
            boost.setSampleRate(rate);
            boost.process(pack(32767, -32768));
            for (unsigned i = 0; i < rate / 10; ++i) assert(boost.process(0) == 0);
            const auto recovered = boost.process(pack(256, 256));
            assert(std::abs(unpack(recovered) - expected) < 2);
        }
    }
    boost.setLevel(255);
    assert(boost.level() == audio::Boost::maxLevel);
    assert(boost.decibels() == 90);
    boost.reset();
    const auto minimum = boost.process(pack(1, -1));
    assert(unpack(minimum >> 16) == audio::Boost::ceiling);
    assert(unpack(minimum) == -audio::Boost::ceiling);
    boost.setLevel(0);
    assert(boost.process(pack(32767, -32768)) == pack(32767, -32768));
    std::cout << "Audio boost: gain, bypass, stereo linking, transient bounds, recovery and silence checks passed\n";
}
