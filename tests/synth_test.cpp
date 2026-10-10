#include <cstdio>
#include <vector>
#include "../src/dsp/Effects.h"
#include "../src/dsp/Lfo.h"
#include "../src/dsp/ModMatrix.h"
#include "../src/dsp/OscPair.h"
#include "../src/gen/TableGenerator.h"
using namespace gh;

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } } while (0)

int main() {
    Wavetable A, B;
    A.build(generateTable(1, Style::Glass));
    B.build(generateTable(2, Style::Metallic));

    // --- Oscillators: sync makes B periodic with A (1/512 is exact in float) ---
    {
        OscParams p; p.incA = 1.f / 512.f; p.incB = 3.3f / 512.f; p.mix = 0.5f; p.mipA = p.mipB = 2;
        auto run = [&](bool sync) {
            OscPair o; p.sync = sync;
            std::vector<float> v(2048);
            for (auto& x : v) x = o.process(A, B, p);
            double d = 0;
            for (int n = 512; n < 1024; ++n) d = std::max(d, double(std::abs(v[n] - v[n + 512])));
            return d;
        };
        CHECK(run(true) < 1e-4, "hard sync output repeats every master cycle");
        CHECK(run(false) > 0.05, "without sync the oscillators drift apart");
    }
    // --- FM changes the sound; ring mod == A*B at full depth ---
    {
        OscParams p; p.incA = 1.f / 512.f; p.incB = 2.f / 512.f; p.mix = 1.f; p.mipA = p.mipB = 2;
        OscPair o1, o2; double diff = 0;
        OscParams q = p; q.fm = 0.5f;
        for (int n = 0; n < 512; ++n) diff = std::max(diff, double(std::abs(o1.process(A, B, p) - o2.process(A, B, q))));
        CHECK(diff > 0.05, "FM alters the waveform");

        OscParams r = p; r.ring = 1.f; r.mix = 0.f;
        OscPair o3; float phA = 0, phB = 0; double err = 0;
        for (int n = 0; n < 512; ++n) {
            const float expect = 1.4f * A.read(phA, 0, 2) * B.read(phB, 0, 2);
            err = std::max(err, double(std::abs(o3.process(A, B, r) - expect)));
            phA += r.incA; phB += r.incB; phA -= std::floor(phA); phB -= std::floor(phB);
        }
        CHECK(err < 1e-4, "ring mod equals 1.4*A*B");
    }
    // --- LFO ---
    {
        Lfo l; l.trigger(5);
        l.phase = 0.25f; CHECK(std::abs(l.value(LfoSine) - 1.f) < 1e-5f, "sine peak");
        l.phase = 0.25f; CHECK(std::abs(l.value(LfoTriangle) - 1.f) < 1e-5f, "triangle peak");
        l.phase = 0.75f; CHECK(std::abs(l.value(LfoTriangle) + 1.f) < 1e-5f, "triangle trough");
        l.phase = 0.9f;  CHECK(l.value(LfoSquare) == -1.f, "square low half");
        l.phase = 0.1f;
        const float h0 = l.value(LfoSampleHold);
        l.advance(0.4f); CHECK(l.value(LfoSampleHold) == h0, "S&H holds within a cycle");
        l.advance(0.7f); CHECK(l.value(LfoSampleHold) != h0, "S&H changes on wrap");
        Lfo a, b; a.trigger(9); b.trigger(9);
        CHECK(a.value(LfoSampleHold) == b.value(LfoSampleHold), "same trigger seed -> same S&H");
    }
    // --- Mod matrix ---
    {
        float src[kNumSrc] = {}; src[SrcLfo1] = 0.5f; src[SrcEnv] = 1.f;
        ModSlot s[kNumSlots];
        s[0] = {SrcLfo1, DstCutoff, 1.f}; s[1] = {SrcEnv, DstCutoff, -0.25f};
        s[2] = {SrcEnv, DstNone, 1.f};    s[3] = {SrcNone, DstMix, 1.f};
        float d[kNumDst];
        applyMatrix(s, kNumSlots, src, d);
        CHECK(std::abs(d[DstCutoff] - 0.25f) < 1e-6f, "slots to the same destination sum");
        CHECK(d[DstMix] == 0.f && d[DstPosA] == 0.f, "empty slots do nothing");
    }
    // --- Delay: echo arrives on time, ping-pongs, and decays ---
    {
        PingPongDelay dly; dly.prepare(48000.0); dly.set(100.f, 0.5f, 1.f); dly.snap();
        const int t = 4800;
        std::vector<float> oL(t * 6), oR(t * 6);
        for (int n = 0; n < t * 6; ++n) {
            float l = n == 0 ? 1.f : 0.f, r = l;
            dly.process(l, r); oL[n] = l; oR[n] = r;
        }
        CHECK(std::abs(oL[t] - 1.f) < 0.02f, "first echo at the set time on L");
        float rPeak = 0; for (int n = 2 * t - 50; n < 2 * t + 50; ++n) rPeak = std::max(rPeak, std::abs(oR[n]));
        CHECK(rPeak > 0.05f, "second echo lands on R (ping-pong)");
        bool finite = true; for (float v : oL) if (!std::isfinite(v)) finite = false;
        CHECK(finite, "delay output finite");
        float tail = 0; for (int n = t * 5; n < t * 6; ++n) tail = std::max(tail, std::abs(oL[n]));
        CHECK(tail < 0.5f, "feedback decays");
    }
    // --- Chorus: bounded and finite ---
    {
        Chorus c; c.prepare(48000.0); c.set(0.8f, 1.f, 1.f);
        float mx = 0; bool finite = true;
        for (int n = 0; n < 48000; ++n) {
            float l = std::sin(0.05f * float(n)), r = l; c.process(l, r);
            if (!std::isfinite(l) || !std::isfinite(r)) finite = false;
            mx = std::max(mx, std::abs(l));
        }
        CHECK(finite && mx < 2.f, "chorus stable");
    }
    std::printf(fails ? "%d test(s) failed\n" : "all synth tests passed\n", fails);
    return fails;
}
