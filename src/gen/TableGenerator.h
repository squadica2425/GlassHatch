// Seeded, deterministic generation of additive wavetables.
// (seed, style) -> the same table on every machine; that is the "infinite" part.
#pragma once
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include "../dsp/Wavetable.h"

namespace gh {

enum class Style : int { Glass = 0, SubGrowl, Metallic, Vocal, Count };

struct Recipe {
    float tiltMin, tiltMax;        // spectral slope: amp ~ h^-tilt
    float evenCutMin, evenCutMax;  // attenuate even harmonics (hollow / odd-heavy)
    int   formantsMin, formantsMax;
    float formCenterMax;           // formant centre range, in harmonic numbers
    float formWidthMax;
    float highCutMin, highCutMax;  // harmonic where the spectrum rolls off
    float fundBoost;               // extra weight on harmonic 1 (sub weight)
    float phaseScramble;           // 0 = sine-phase, 1 = fully random phases
};

inline const Recipe& recipeFor(Style s) {
    static const Recipe r[] = {
        /* Glass    */ {0.6f, 1.1f, 0.0f, 0.3f, 2, 3, 60.f, 6.f, 80.f, 300.f, 1.0f, 0.2f},
        /* SubGrowl */ {1.2f, 2.0f, 0.0f, 0.6f, 1, 2, 14.f, 4.f, 20.f, 120.f, 2.0f, 0.1f},
        /* Metallic */ {0.3f, 0.8f, 0.5f, 0.9f, 3, 5, 120.f, 8.f, 150.f, 500.f, 1.0f, 0.9f},
        /* Vocal    */ {0.8f, 1.3f, 0.0f, 0.2f, 3, 3, 40.f, 5.f, 60.f, 200.f, 1.0f, 0.1f},
    };
    return r[int(s)];
}

namespace detail {
// Platform-independent uniform float (std::uniform_real_distribution is not portable).
struct Rng {
    std::mt19937 g;
    explicit Rng(std::uint32_t seed) : g(seed) {}
    float u() { return float(g() >> 8) * (1.0f / 16777216.0f); }
    float range(float a, float b) { return a + (b - a) * u(); }
    int   irange(int a, int b) { return a + int(u() * float(b - a + 1)) % (b - a + 1); }
};
}  // namespace detail

inline Partials makeKeyframe(detail::Rng& rng, const Recipe& rc) {
    Partials p;
    const float tilt    = rng.range(rc.tiltMin, rc.tiltMax);
    const float evenCut = rng.range(rc.evenCutMin, rc.evenCutMax);
    const float highCut = rng.range(rc.highCutMin, rc.highCutMax);
    const int   nForm   = std::min(rng.irange(rc.formantsMin, rc.formantsMax), 8);
    float c[8], w[8], g[8];
    for (int i = 0; i < nForm; ++i) {
        c[i] = rng.range(2.f, rc.formCenterMax);
        w[i] = rng.range(1.f, rc.formWidthMax);
        g[i] = rng.range(0.5f, 3.0f);
    }
    for (int h = 1; h <= kMaxPartials; ++h) {
        float a = std::pow(float(h), -tilt);
        float bump = 1.f;
        for (int i = 0; i < nForm; ++i) {
            const float d = (float(h) - c[i]) / w[i];
            bump += g[i] * std::exp(-d * d);
        }
        a *= bump;
        if (h % 2 == 0) a *= (1.f - evenCut);
        if (h == 1) a *= rc.fundBoost;
        if (float(h) > highCut) a *= std::exp(-(float(h) - highCut) / (0.25f * highCut));
        a *= 1.f + 0.3f * rng.range(-1.f, 1.f);  // jitter keeps tables from sounding synthetic-clean
        p.amp[h - 1] = a;
    }
    return p;
}

inline std::vector<Partials> generateTable(std::uint32_t seed, Style style,
                                           int numFrames = 32, int numKeys = 4) {
    detail::Rng rng(seed);
    const Recipe& rc = recipeFor(style);
    std::vector<Partials> keys;
    for (int k = 0; k < numKeys; ++k) keys.push_back(makeKeyframe(rng, rc));

    // Phases are fixed per table so morphing doesn't produce phase cancellation mush.
    std::array<float, kMaxPartials> phase{};
    for (int h = 2; h <= kMaxPartials; ++h)
        phase[h - 1] = rc.phaseScramble * rng.range(0.f, 2.f * kPi);

    std::vector<Partials> frames(numFrames);
    for (int f = 0; f < numFrames; ++f) {
        const float pos = float(f) / float(numFrames - 1) * float(numKeys - 1);
        const int k0 = std::min(int(pos), numKeys - 2);
        float t = pos - float(k0);
        t = t * t * (3.f - 2.f * t);  // smoothstep
        for (int h = 0; h < kMaxPartials; ++h) {
            frames[f].amp[h] = keys[k0].amp[h] + (keys[k0 + 1].amp[h] - keys[k0].amp[h]) * t;
            frames[f].phase[h] = phase[h];
        }
    }
    return frames;
}

}  // namespace gh
