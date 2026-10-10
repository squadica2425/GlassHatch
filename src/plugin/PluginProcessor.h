#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../dsp/Effects.h"
#include "Voice.h"

namespace gh {

class GlassHatchProcessor : public juce::AudioProcessor {
public:
    GlassHatchProcessor();
    ~GlassHatchProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GlassHatch"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    std::shared_ptr<const TableSet> getTables() const { return std::atomic_load(&ctx.tables); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void requestRegeneration(int seed, int style);

    SynthContext ctx;
    juce::Synthesiser synth;
    Chorus chorus;
    PingPongDelay delay;
    juce::Reverb reverb;
    int lastSeed = -1, lastStyle = -1;
    juce::ThreadPool pool{1};  // keep last: destroyed first, waits for running jobs
};

}  // namespace gh
