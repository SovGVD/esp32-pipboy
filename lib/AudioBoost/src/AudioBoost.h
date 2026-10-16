#pragma once

#include <algorithm>
#include <cstdint>

namespace audio {
// Additional digital gain after the library's volume/EQ stage. A stereo-linked
// limiter has immediate attack and at most 100 ms linear gain recovery, keeping
// transients within signed 16-bit range without independently shifting channels.
// Level zero is an exact bypass, including full-scale samples.
class Boost {
public:
    // +90 dB can bring a single 16-bit LSB to the limiter ceiling. More
    // requested gain cannot produce a higher bounded output for such a sample.
    static constexpr uint8_t maxLevel = 15;
    static constexpr int32_t ceiling = 30000;

    void setLevel(uint8_t level) {
        level_ = level > maxLevel ? maxLevel : level;
        gain_ = std::min(gain_, requestedGain());
        updateRelease();
    }
    uint8_t level() const { return level_; }
    uint8_t decibels() const { return level_ * 6; }
    void setSampleRate(uint32_t rate) {
        if (rate >= 8000 && rate <= 192000 && rate != sampleRate_) {
            sampleRate_ = rate;
            updateRelease();
        }
    }
    void reset() { gain_ = requestedGain(); }

    // ESP32-audioI2S 3.0.8 packs left in bits 31..16 and right in 15..0.
    uint32_t process(uint32_t packed) {
        if (!level_) return packed;
        const int32_t left = signedSample(packed >> 16);
        const int32_t right = signedSample(packed & 0xffff);
        const int32_t peak = std::max(left < 0 ? -left : left, right < 0 ? -right : right);
        gain_ = std::min(requestedGain(), gain_ + release_);
        if (peak && static_cast<int64_t>(peak) * gain_ > ceiling * unity) gain_ = ceiling * unity / peak;
        const int32_t outLeft = static_cast<int64_t>(left) * gain_ / unity;
        const int32_t outRight = static_cast<int64_t>(right) * gain_ / unity;
        return (static_cast<uint32_t>(outLeft) & 0xffff) << 16 |
               (static_cast<uint32_t>(outRight) & 0xffff);
    }

private:
    static constexpr int32_t unity = 4096; // Q12 gain; PCM products require int64_t.
    static int32_t signedSample(uint32_t bits) {
        return bits >= 0x8000 ? static_cast<int32_t>(bits) - 65536 : static_cast<int32_t>(bits);
    }
    int32_t requestedGain() const {
        // round(4096 * 10^(dB / 20)): 0..+90 dB in 6 dB steps.
        constexpr int32_t gains[] = {
            4096, 8173, 16306, 32536, 64917, 129527, 258440, 515656,
            1028869, 2052863, 4096000, 8172594, 16306470, 32535684,
            64917225, 129526893
        };
        return gains[level_];
    }
    void updateRelease() {
        const uint32_t frames = sampleRate_ / 10;
        const int32_t minimumGain = ceiling * unity / 32768;
        release_ = std::max<int32_t>(1, (requestedGain() - minimumGain + frames - 1) / frames);
    }
    uint8_t level_ = 0;
    uint32_t sampleRate_ = 44100;
    int32_t gain_ = unity;
    int32_t release_ = 1;
};
}
