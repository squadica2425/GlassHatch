#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "WavetableDisplay.h"

namespace gh {

class GlassHatchEditor : public juce::AudioProcessorEditor {
public:
    explicit GlassHatchEditor(GlassHatchProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    void addKnob(Knob&, const juce::String& id, const juce::String& text, juce::Colour c);
    static void place(Knob&, juce::Rectangle<int> r);

    GlassHatchProcessor& proc;
    juce::Colour colA{0xff4fd1ff}, colB{0xffff5fa8}, colFx{0xffffb347};

    WavetableDisplay dispA, dispB;
    Knob posA, posB, detune, mix, cutoff, res, drive, atk, dec, sus, rel;

    juce::Slider seedSlider;
    juce::TextButton dice{"Randomize"};
    juce::ComboBox styleBox;
    juce::Label title;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seedAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> styleAtt;

    juce::Rectangle<int> panelA, panelB, panelMix, panelFilter, panelEnv;
};

}  // namespace gh
