// Two wavetable oscillators with hard sync, FM and ring modulation.
// A is the master: it FMs B, resets B's phase (sync) and ring-mods with it.
#pragma once
#include <cmath>
#include "Wavetable.h"

namespace gh {

struct OscParams {
    float posA = 0, posB = 0;   // wavetable positions 0..1
    float mix = 0.5f;           // 0 = A only, 1 = B only
    float fm = 0;               // 0..1, A -> B phase modulation
    float ring = 0;             // 0..1, crossfade to A*B
    bool  sync = false;         // B restarts whenever A wraps
    float incA = 0, incB = 0;   // phase increments (freq / sampleRate)
    int   mipA = 0, mipB = 0;
};

struct OscPair {
    float phA = 0, phB = 0;

    void reset() { phA = phB = 0.f; }

    float process(const Wavetable& A, const Wavetable& B, const OscParams& p) {
        const float sA = A.read(phA, p.posA, p.mipA);
        float pm = phB + p.fm * 1.5f * sA;  // up to +-1.5 cycles of phase modulation
        pm -= std::floor(pm);
        const float sB = B.read(pm, p.posB, p.mipB);

        float out = (1.f - p.mix) * sA + p.mix * sB;
        if (p.ring > 0.f) out = out * (1.f - p.ring) + 1.4f * sA * sB * p.ring;

        phA += p.incA;
        phB += p.incB;
        if (phA >= 1.f) {
            phA -= 1.f;
            if (p.sync && p.incA > 0.f) phB = phA * (p.incB / p.incA);  // sub-sample accurate reset
        }
        phB -= std::floor(phB);
        return out;
    }
};

}  // namespace gh
