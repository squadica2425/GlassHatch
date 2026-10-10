#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace gh {

using F  = juce::AudioParameterFloat;
using NR = juce::NormalisableRange<float>;
using PID = juce::ParameterID;

juce::AudioProcessorValueTreeState::ParameterLayout GlassHatchProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<F>(PID{"posA", 1}, "Osc A Position", NR(0.f, 1.f), 0.f));
    l.add(std::make_unique<F>(PID{"posB", 1}, "Osc B Position", NR(0.f, 1.f), 0.f));
    l.add(std::make_unique<F>(PID{"mix", 1}, "A/B Mix", NR(0.f, 1.f), 0.35f));
    l.add(std::make_unique<F>(PID{"detune", 1}, "Osc B Detune", NR(-100.f, 100.f), 7.f));
    l.add(std::make_unique<F>(PID{"cutoff", 1}, "Cutoff", NR(20.f, 18000.f, 0.f, 0.3f), 8000.f));
    l.add(std::make_unique<F>(PID{"res", 1}, "Resonance", NR(0.7f, 10.f), 0.8f));
    l.add(std::make_unique<F>(PID{"drive", 1}, "Drive", NR(0.f, 1.f), 0.f));
    l.add(std::make_unique<F>(PID{"atk", 1}, "Attack", NR(0.001f, 5.f, 0.f, 0.4f), 0.005f));
    l.add(std::make_unique<F>(PID{"dec", 1}, "Decay", NR(0.001f, 5.f, 0.f, 0.4f), 0.3f));
    l.add(std::make_unique<F>(PID{"sus", 1}, "Sustain", NR(0.f, 1.f), 0.7f));
    l.add(std::make_unique<F>(PID{"rel", 1}, "Release", NR(0.001f, 8.f, 0.f, 0.4f), 0.3f));
    l.add(std::make_unique<juce::AudioParameterInt>(PID{"seed", 1}, "Seed", 0, 99999, 1));
    l.add(std::make_unique<juce::AudioParameterChoice>(
        PID{"style", 1}, "Style", juce::StringArray{"Glass", "Sub Growl", "Metallic", "Vocal"}, 0));
    return l;
}

GlassHatchProcessor::GlassHatchProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout()) {
    ctx.apvts = &apvts;
    lastSeed = int(apvts.getRawParameterValue("seed")->load());
    lastStyle = int(apvts.getRawParameterValue("style")->load());
    ctx.tables = TableSet::make(std::uint32_t(lastSeed), Style(lastStyle));
    for (int i = 0; i < 8; ++i) synth.addVoice(new Voice(ctx));
    synth.addSound(new Sound());
}

void GlassHatchProcessor::prepareToPlay(double sampleRate, int) {
    synth.setCurrentPlaybackSampleRate(sampleRate);
}

bool GlassHatchProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void GlassHatchProcessor::requestRegeneration(int seed, int style) {
    pool.addJob([this, seed, style] {
        std::shared_ptr<const TableSet> t = TableSet::make(std::uint32_t(seed), Style(style));
        std::atomic_store(&ctx.tables, t);
    });
}

void GlassHatchProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int seed = int(apvts.getRawParameterValue("seed")->load());
    const int style = int(apvts.getRawParameterValue("style")->load());
    if (seed != lastSeed || style != lastStyle) {
        lastSeed = seed; lastStyle = style;
        requestRegeneration(seed, style);
    }
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

void GlassHatchProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void GlassHatchProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* GlassHatchProcessor::createEditor() { return new GlassHatchEditor(*this); }

}  // namespace gh

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new gh::GlassHatchProcessor(); }
