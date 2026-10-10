#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Knob.h"
#include "PluginProcessor.h"
#include "WavetableDisplay.h"

namespace gh {

class GlassHatchEditor : public juce::AudioProcessorEditor {
public:
    explicit GlassHatchEditor(GlassHatchProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    GlassHatchProcessor& proc;

    WavetableDisplay dispA, dispB;
    Knob posA, posB, detune;

    juce::Slider seedSlider;
    juce::TextButton dice{"Randomize"};
    juce::ComboBox styleBox;
    juce::Label title;
    std::unique_ptr<APVTS::SliderAttachment> seedAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> styleAtt;

    juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};
    juce::Rectangle<int> panelA, panelB;
};

}  // namespace gh
