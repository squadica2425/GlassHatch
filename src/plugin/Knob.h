#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace gh {

using APVTS = juce::AudioProcessorValueTreeState;

struct Knob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<APVTS::SliderAttachment> att;

    void init(juce::Component& parent, APVTS& apvts, const juce::String& id,
              const juce::String& text, juce::Colour c) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
        slider.setColour(juce::Slider::rotarySliderFillColourId, c);
        slider.setColour(juce::Slider::thumbColourId, c);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.8f));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        parent.addAndMakeVisible(slider);
        parent.addAndMakeVisible(label);
        att = std::make_unique<APVTS::SliderAttachment>(apvts, id, slider);
    }
    void place(juce::Rectangle<int> r) {
        label.setBounds(r.removeFromTop(16));
        slider.setBounds(r);
    }
};

inline void drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name, juce::Colour c) {
    g.setColour(juce::Colour(0xff10131d));
    g.fillRoundedRectangle(r.toFloat(), 8.f);
    g.setColour(c.withAlpha(0.35f));
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 8.f, 1.f);
    g.setColour(c);
    g.setFont(juce::FontOptions(12.f, juce::Font::bold));
    g.drawText(name, r.reduced(10, 6).removeFromTop(14), juce::Justification::topLeft);
}

// Filled combo box bound to a choice parameter.
struct ChoiceBox {
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
    void init(juce::Component& parent, APVTS& apvts, const juce::String& id, const juce::StringArray& items) {
        box.addItemList(items, 1);
        parent.addAndMakeVisible(box);
        att = std::make_unique<APVTS::ComboBoxAttachment>(apvts, id, box);
    }
};

}  // namespace gh
