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
    l.add(std::make_unique<juce::AudioParameterBool>(PID{"sync", 1}, "Hard Sync (A->B)", false));
    l.add(std::make_unique<F>(PID{"fm", 1}, "FM (A->B)", NR(0.f, 1.f), 0.f));
    l.add(std::make_unique<F>(PID{"ring", 1}, "Ring Mod", NR(0.f, 1.f), 0.f));

    juce::StringArray shapes, srcs, dsts;
    for (auto* n : kLfoShapeNames) shapes.add(n);
    for (auto* n : kSrcNames) srcs.add(n);
    for (auto* n : kDstNames) dsts.add(n);
    for (int i = 1; i <= 3; ++i) {
        l.add(std::make_unique<F>(PID{"lfo" + juce::String(i) + "Rate", 1}, "LFO " + juce::String(i) + " Rate",
                                  NR(0.05f, 30.f, 0.f, 0.4f), 0.5f * float(i)));
        l.add(std::make_unique<juce::AudioParameterChoice>(PID{"lfo" + juce::String(i) + "Shape", 1},
                                                           "LFO " + juce::String(i) + " Shape", shapes, 0));
    }
    // Default routings are wired up but silent (amount 0): LFO1->PosA, LFO2->Cutoff, Env->Cutoff.
    const int defSrc[kNumSlots] = {SrcLfo1, SrcLfo2, SrcEnv, SrcNone, SrcNone, SrcNone};
    const int defDst[kNumSlots] = {DstPosA, DstCutoff, DstCutoff, DstNone, DstNone, DstNone};
    for (int i = 0; i < kNumSlots; ++i) {
        const auto n = "m" + juce::String(i + 1);
        const auto h = "Mod " + juce::String(i + 1);
        l.add(std::make_unique<juce::AudioParameterChoice>(PID{n + "Src", 1}, h + " Source", srcs, defSrc[i]));
        l.add(std::make_unique<juce::AudioParameterChoice>(PID{n + "Dst", 1}, h + " Dest", dsts, defDst[i]));
        l.add(std::make_unique<F>(PID{n + "Amt", 1}, h + " Amount", NR(-1.f, 1.f), 0.f));
    }

    l.add(std::make_unique<F>(PID{"chRate", 1}, "Chorus Rate", NR(0.1f, 3.f), 0.6f));
    l.add(std::make_unique<F>(PID{"chDepth", 1}, "Chorus Depth", NR(0.f, 1.f), 0.5f));
    l.add(std::make_unique<F>(PID{"chMix", 1}, "Chorus Mix", NR(0.f, 1.f), 0.f));
    l.add(std::make_unique<F>(PID{"dlTime", 1}, "Delay Time", NR(20.f, 1500.f, 0.f, 0.5f), 375.f));
    l.add(std::make_unique<F>(PID{"dlFb", 1}, "Delay Feedback", NR(0.f, 0.95f), 0.45f));
    l.add(std::make_unique<F>(PID{"dlMix", 1}, "Delay Mix", NR(0.f, 1.f), 0.f));
    l.add(std::make_unique<F>(PID{"rvSize", 1}, "Reverb Size", NR(0.f, 1.f), 0.6f));
    l.add(std::make_unique<F>(PID{"rvMix", 1}, "Reverb Mix", NR(0.f, 1.f), 0.f));

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
    chorus.prepare(sampleRate);
    delay.prepare(sampleRate);
    reverb.setSampleRate(sampleRate);
    reverb.reset();
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
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isController() && m.getControllerNumber() == 1)
            ctx.modWheel.store(float(m.getControllerValue()) / 127.f);
    }
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    // ---- effects: chorus -> ping-pong delay -> reverb ----
    auto val = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);
    const float chMix = val("chMix"), dlMix = val("dlMix"), rvMix = val("rvMix");
    if (chMix > 0.001f) {
        chorus.set(val("chRate"), val("chDepth"), chMix);
        for (int i = 0; i < n; ++i) chorus.process(L[i], R[i]);
    }
    if (dlMix > 0.001f) {
        delay.set(val("dlTime"), val("dlFb"), dlMix);
        for (int i = 0; i < n; ++i) delay.process(L[i], R[i]);
    }
    if (rvMix > 0.001f) {
        juce::Reverb::Parameters rp;
        rp.roomSize = val("rvSize"); rp.damping = 0.5f; rp.width = 1.f;
        rp.wetLevel = rvMix * 0.4f; rp.dryLevel = 1.f; rp.freezeMode = 0.f;
        reverb.setParameters(rp);
        reverb.processStereo(L, R, n);
    }
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
