// Stereo chorus and dubby ping-pong delay. JUCE-free. (Reverb uses juce::Reverb in the processor.)
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "Wavetable.h"  // kPi

namespace gh {

struct DelayLine {
    std::vector<float> b;
    int w = 0;
    void prepare(int n) { b.assign(std::size_t(std::max(n, 4)), 0.f); w = 0; }
    // Call read() BEFORE push() for the current sample: read(d) returns the input from d samples ago.
    float read(float d) const {
        const int size = int(b.size());
        float r = float(w) - d;
        while (r < 0.f) r += float(size);
        const int i0 = int(r) % size, i1 = (i0 + 1) % size;
        const float f = r - std::floor(r);
        return b[std::size_t(i0)] + (b[std::size_t(i1)] - b[std::size_t(i0)]) * f;
    }
    void push(float x) { b[std::size_t(w)] = x; w = (w + 1) % int(b.size()); }
};

struct Chorus {
    DelayLine L, R;
    float sr = 44100.f, phase = 0.f, rate = 0.6f, depth = 0.5f, mix = 0.f;
    void prepare(double sampleRate) { sr = float(sampleRate); L.prepare(int(sr * 0.05f) + 4); R.prepare(int(sr * 0.05f) + 4); phase = 0.f; }
    void set(float r, float d, float m) { rate = r; depth = d; mix = m; }
    void process(float& l, float& r) {
        phase += rate / sr; phase -= std::floor(phase);
        const float mL = 0.5f + 0.5f * std::sin(2.f * kPi * phase);
        const float mR = 0.5f + 0.5f * std::sin(2.f * kPi * (phase + 0.25f));
        const float base = 0.012f * sr, dep = depth * 0.006f * sr;
        const float dl = L.read(base + dep * mL), dr = R.read(base + dep * mR);
        L.push(l); R.push(r);
        l = (1.f - 0.5f * mix) * l + 0.7f * mix * dl;
        r = (1.f - 0.5f * mix) * r + 0.7f * mix * dr;
    }
};

struct PingPongDelay {
    DelayLine L, R;
    float sr = 44100.f, cur = 0.f, target = 0.f, fb = 0.4f, mix = 0.f, lpL = 0.f, lpR = 0.f;
    void prepare(double sampleRate) {
        sr = float(sampleRate);
        L.prepare(int(sr * 2.0f)); R.prepare(int(sr * 2.0f));
        cur = target = 0.375f * sr; lpL = lpR = 0.f;
    }
    void set(float ms, float feedback, float m) {
        target = std::clamp(ms * 0.001f * sr, 8.f, sr * 1.9f);
        fb = std::clamp(feedback, 0.f, 0.95f); mix = m;
    }
    void snap() { cur = target; }  // jump to the target time (no glide)
    void process(float& l, float& r) {
        cur += (target - cur) * 0.0005f;  // slow glide: time changes give the classic dub pitch-sweep
        const float dl = L.read(cur), dr = R.read(cur);
        lpL += 0.4f * (dl - lpL);          // darken each repeat
        lpR += 0.4f * (dr - lpR);
        const float mono = 0.5f * (l + r);
        L.push(mono + std::tanh(fb * lpR));  // cross-feed = ping-pong
        R.push(std::tanh(fb * lpL));
        l += mix * dl;
        r += mix * dr;
    }
};

}  // namespace gh
