#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../dsp/Lfo.h"
#include "../dsp/ModMatrix.h"
#include "../dsp/OscPair.h"
#include "TableSet.h"

namespace gh {

// Shared between processor, voices and editor. `tables` is swapped atomically
// by the background generator; the audio thread only ever loads it.
struct SynthContext {
    juce::AudioProcessorValueTreeState* apvts = nullptr;
    std::shared_ptr<const TableSet> tables;
    std::atomic<float> modWheel{0.f};  // CC1, 0..1
};

struct Sound : juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Voice : public juce::SynthesiserVoice {
public:
    static constexpr int kHop = 16;  // modulation is computed every 16 samples

    explicit Voice(SynthContext& c) : ctx(c) {
        auto p = [&](const juce::String& id) { return ctx.apvts->getRawParameterValue(id); };
        posA = p("posA"); posB = p("posB"); mix = p("mix"); detune = p("detune");
        fm = p("fm"); ring = p("ring"); sync = p("sync");
        cutoff = p("cutoff"); res = p("res"); drive = p("drive");
        atk = p("atk"); dec = p("dec"); sus = p("sus"); rel = p("rel");
        for (int i = 0; i < 3; ++i) {
            lfoRate[i] = p("lfo" + juce::String(i + 1) + "Rate");
            lfoShape[i] = p("lfo" + juce::String(i + 1) + "Shape");
        }
        for (int i = 0; i < kNumSlots; ++i) {
            const auto n = "m" + juce::String(i + 1);
            mSrc[i] = p(n + "Src"); mDst[i] = p(n + "Dst"); mAmt[i] = p(n + "Amt");
        }
        filt.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<Sound*>(s) != nullptr; }

    void setCurrentPlaybackSampleRate(double sr) override {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate(sr);
        env.setSampleRate(sr);
        juce::dsp::ProcessSpec spec{sr, 512u, 1u};
        filt.prepare(spec);
    }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int) override {
        tables = std::atomic_load(&ctx.tables);
        note = midiNote;
        hz = float(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        level = velocity;
        osc.reset();
        filt.reset();
        rng = rng * 1664525u + 1013904223u + std::uint32_t(midiNote);
        noteRandom = xorshiftRand(rng);
        for (int i = 0; i < 3; ++i) lfo[i].trigger(rng + std::uint32_t(i));  // free-running LFOs retrigger per note
        lastEnv = 0.f;
        env.noteOn();
    }

    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) env.noteOff();
        else { env.reset(); clearCurrentNote(); }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override {
        if (!tables || getCurrentlyPlayingNote() < 0) return;
        const float sr = float(getSampleRate());
        env.setParameters({atk->load(), dec->load(), sus->load(), rel->load()});

        ModSlot slots[kNumSlots];
        for (int i = 0; i < kNumSlots; ++i)
            slots[i] = {int(mSrc[i]->load()), int(mDst[i]->load()), mAmt[i]->load()};
        int shape[3];
        for (int i = 0; i < 3; ++i) shape[i] = int(lfoShape[i]->load());

        OscParams op;
        op.sync = sync->load() > 0.5f;
        const float detuneCents = detune->load();
        const float baseDrive = drive->load();

        int i = 0;
        while (i < num) {
            const int n = std::min(kHop, num - i);

            // ---- control-rate modulation ----
            float src[kNumSrc] = {};
            for (int k = 0; k < 3; ++k) src[SrcLfo1 + k] = lfo[k].value(shape[k]);
            src[SrcEnv] = lastEnv;
            src[SrcVel] = level;
            src[SrcWheel] = ctx.modWheel.load();
            src[SrcKey] = juce::jlimit(-1.f, 1.f, float(note - 60) / 36.f);
            src[SrcRandom] = noteRandom;
            float d[kNumDst];
            applyMatrix(slots, kNumSlots, src, d);

            auto c01 = [](float v) { return juce::jlimit(0.f, 1.f, v); };
            op.posA = c01(posA->load() + d[DstPosA]);
            op.posB = c01(posB->load() + d[DstPosB]);
            op.fm   = c01(fm->load() + d[DstFm]);
            op.ring = c01(ring->load() + d[DstRing]);
            op.mix  = c01(mix->load() + d[DstMix]);
            const float dr = c01(baseDrive + d[DstDrive]);
            const float semis = d[DstPitch] * 12.f;
            const float hzMod = hz * std::pow(2.f, semis / 12.f);
            op.incA = hzMod / sr;
            op.incB = hzMod * std::pow(2.f, detuneCents / 1200.f) / sr;
            op.mipA = Wavetable::mipForIncrement(op.incA);
            op.mipB = Wavetable::mipForIncrement(op.incB);
            if (op.fm > 0.25f) op.mipB = std::min(op.mipB + 1, kMipLevels - 1);  // FM spreads the spectrum
            filt.setCutoffFrequency(juce::jlimit(20.f, 18000.f, cutoff->load() * std::pow(2.f, d[DstCutoff] * 4.f)));
            filt.setResonance(juce::jlimit(0.7f, 10.f, res->load() + d[DstReso] * 5.f));
            const float k = 1.f + dr * 6.f, kNorm = 1.f / std::tanh(k);

            // ---- audio-rate loop ----
            for (int j = 0; j < n; ++j) {
                const float e = env.getNextSample();
                float s = osc.process(tables->wtA, tables->wtB, op);
                s = std::tanh(s * k) * kNorm;
                s = filt.processSample(0, s) * e * level * 0.3f;
                for (int ch = 0; ch < out.getNumChannels(); ++ch) out.addSample(ch, start + i + j, s);
                lastEnv = e;
                if (!env.isActive()) { clearCurrentNote(); return; }
            }
            for (int kk = 0; kk < 3; ++kk) lfo[kk].advance(lfoRate[kk]->load() * float(n) / sr);
            i += n;
        }
    }

private:
    SynthContext& ctx;
    std::shared_ptr<const TableSet> tables;
    std::atomic<float> *posA, *posB, *mix, *detune, *fm, *ring, *sync, *cutoff, *res, *drive,
        *atk, *dec, *sus, *rel;
    std::atomic<float> *lfoRate[3], *lfoShape[3], *mSrc[kNumSlots], *mDst[kNumSlots], *mAmt[kNumSlots];
    juce::ADSR env;
    juce::dsp::StateVariableTPTFilter<float> filt;
    OscPair osc;
    Lfo lfo[3];
    std::uint32_t rng = 12345u;
    int note = 60;
    float hz = 440.f, level = 1.f, lastEnv = 0.f, noteRandom = 0.f;
};

}  // namespace gh
