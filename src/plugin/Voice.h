#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "TableSet.h"

namespace gh {

// Shared between processor, voices and editor. `tables` is swapped atomically
// by the background generator; the audio thread only ever loads it.
struct SynthContext {
    juce::AudioProcessorValueTreeState* apvts = nullptr;
    std::shared_ptr<const TableSet> tables;
};

struct Sound : juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Voice : public juce::SynthesiserVoice {
public:
    explicit Voice(SynthContext& c) : ctx(c) {
        auto p = [&](const char* id) { return ctx.apvts->getRawParameterValue(id); };
        posA = p("posA"); posB = p("posB"); mix = p("mix"); detune = p("detune");
        cutoff = p("cutoff"); res = p("res"); drive = p("drive");
        atk = p("atk"); dec = p("dec"); sus = p("sus"); rel = p("rel");
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
        hz = float(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        level = velocity;
        phA = phB = 0.f;
        filt.reset();
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
        filt.setCutoffFrequency(cutoff->load());
        filt.setResonance(res->load());

        const float pA = posA->load(), pB = posB->load(), mx = mix->load();
        const float k = 1.f + drive->load() * 6.f, kNorm = 1.f / std::tanh(k);
        const float incA = hz / sr;
        const float incB = hz * std::pow(2.f, detune->load() / 1200.f) / sr;
        const int mA = Wavetable::mipForIncrement(incA), mB = Wavetable::mipForIncrement(incB);

        for (int i = 0; i < num; ++i) {
            const float e = env.getNextSample();
            float s = (1.f - mx) * tables->wtA.read(phA, pA, mA) + mx * tables->wtB.read(phB, pB, mB);
            phA += incA; if (phA >= 1.f) phA -= 1.f;
            phB += incB; if (phB >= 1.f) phB -= 1.f;
            s = std::tanh(s * k) * kNorm;
            s = filt.processSample(0, s) * e * level * 0.3f;
            for (int ch = 0; ch < out.getNumChannels(); ++ch) out.addSample(ch, start + i, s);
            if (!env.isActive()) { clearCurrentNote(); break; }
        }
    }

private:
    SynthContext& ctx;
    std::shared_ptr<const TableSet> tables;
    std::atomic<float> *posA, *posB, *mix, *detune, *cutoff, *res, *drive, *atk, *dec, *sus, *rel;
    juce::ADSR env;
    juce::dsp::StateVariableTPTFilter<float> filt;
    float hz = 440.f, level = 1.f, phA = 0.f, phB = 0.f;
};

}  // namespace gh
