#pragma once
#include <cmath>
#include <cstdint>
#include "Wavetable.h"  // kPi

namespace gh {

enum LfoShape { LfoSine = 0, LfoTriangle, LfoSaw, LfoSquare, LfoSampleHold, kNumLfoShapes };
constexpr const char* kLfoShapeNames[kNumLfoShapes] = {"Sine", "Triangle", "Saw", "Square", "S&H"};

inline float xorshiftRand(std::uint32_t& s) {  // returns -1..1
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return float(s >> 8) * (2.f / 16777216.f) - 1.f;
}

struct Lfo {
    float phase = 0.f, held = 0.f;
    std::uint32_t state = 1u;

    void trigger(std::uint32_t seed) {
        phase = 0.f;
        state = seed * 2654435761u + 1u;
        held = xorshiftRand(state);
    }
    float value(int shape) const {  // -1..1 at the current phase
        switch (shape) {
            case LfoSine:     return std::sin(2.f * kPi * phase);
            case LfoTriangle: return phase < 0.25f ? 4.f * phase : (phase < 0.75f ? 2.f - 4.f * phase : 4.f * phase - 4.f);
            case LfoSaw:      return 2.f * phase - 1.f;
            case LfoSquare:   return phase < 0.5f ? 1.f : -1.f;
            default:          return held;
        }
    }
    void advance(float phaseInc) {
        phase += phaseInc;
        if (phase >= 1.f) { phase -= std::floor(phase); held = xorshiftRand(state); }
    }
};

}  // namespace gh
