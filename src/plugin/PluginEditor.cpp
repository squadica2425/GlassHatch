#include "PluginEditor.h"

namespace gh {

GlassHatchEditor::GlassHatchEditor(GlassHatchProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p),
      dispA(p, false, "posA", colA), dispB(p, true, "posB", colB) {
    setSize(900, 620);

    addAndMakeVisible(dispA);
    addAndMakeVisible(dispB);
    addKnob(posA, "posA", "Position", colA);
    addKnob(posB, "posB", "Position", colB);
    addKnob(detune, "detune", "Detune", colB);
    addKnob(mix, "mix", "A / B", colFx);
    addKnob(cutoff, "cutoff", "Cutoff", colFx);
    addKnob(res, "res", "Reso", colFx);
    addKnob(drive, "drive", "Drive", colFx);
    addKnob(atk, "atk", "A", colFx);
    addKnob(dec, "dec", "D", colFx);
    addKnob(sus, "sus", "S", colFx);
    addKnob(rel, "rel", "R", colFx);

    title.setText("GLASSHATCH", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(title);

    styleBox.addItemList({"Glass", "Sub Growl", "Metallic", "Vocal"}, 1);
    addAndMakeVisible(styleBox);
    styleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "style", styleBox);

    seedSlider.setSliderStyle(juce::Slider::IncDecButtons);
    seedSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 24);
    addAndMakeVisible(seedSlider);
    seedAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "seed", seedSlider);

    dice.onClick = [this] {
        if (auto* s = proc.apvts.getParameter("seed")) {
            s->beginChangeGesture();
            s->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
            s->endChangeGesture();
        }
    };
    addAndMakeVisible(dice);
}

void GlassHatchEditor::addKnob(Knob& k, const juce::String& id, const juce::String& text, juce::Colour c) {
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    k.slider.setColour(juce::Slider::rotarySliderFillColourId, c);
    k.slider.setColour(juce::Slider::thumbColourId, c);
    k.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.8f));
    k.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.label.setText(text, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
    addAndMakeVisible(k.slider);
    addAndMakeVisible(k.label);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, k.slider);
}

void GlassHatchEditor::place(Knob& k, juce::Rectangle<int> r) {
    k.label.setBounds(r.removeFromTop(16));
    k.slider.setBounds(r);
}

void GlassHatchEditor::paint(juce::Graphics& g) {
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff151826), 0.f, 0.f,
                                           juce::Colour(0xff07080d), 0.f, float(getHeight()), false));
    g.fillAll();
    auto panel = [&](juce::Rectangle<int> r, const juce::String& name, juce::Colour c) {
        g.setColour(juce::Colour(0xff10131d));
        g.fillRoundedRectangle(r.toFloat(), 8.f);
        g.setColour(c.withAlpha(0.35f));
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 8.f, 1.f);
        g.setColour(c);
        g.setFont(juce::FontOptions(12.f, juce::Font::bold));
        g.drawText(name, r.reduced(10, 6).removeFromTop(14), juce::Justification::topLeft);
    };
    panel(panelA, "OSC A", colA);
    panel(panelB, "OSC B", colB);
    panel(panelMix, "MIX", colFx);
    panel(panelFilter, "FILTER", colFx);
    panel(panelEnv, "AMP ENVELOPE", colFx);
}

void GlassHatchEditor::resized() {
    auto r = getLocalBounds().reduced(16);

    auto header = r.removeFromTop(40);
    title.setBounds(header.removeFromLeft(220));
    dice.setBounds(header.removeFromRight(100).reduced(0, 6));
    header.removeFromRight(8);
    seedSlider.setBounds(header.removeFromRight(130).reduced(0, 6));
    header.removeFromRight(8);
    styleBox.setBounds(header.removeFromRight(130).reduced(0, 6));
    r.removeFromTop(8);

    // Oscillator panels: big wavetable view + knobs
    auto oscRow = r.removeFromTop(370);
    const int half = (oscRow.getWidth() - 12) / 2;
    panelA = oscRow.removeFromLeft(half);
    oscRow.removeFromLeft(12);
    panelB = oscRow;

    auto layoutOsc = [&](juce::Rectangle<int> pnl, WavetableDisplay& d, Knob& pos, Knob* det) {
        auto inner = pnl.reduced(10);
        inner.removeFromTop(16);
        auto knobs = inner.removeFromBottom(92);
        d.setBounds(inner);
        place(pos, knobs.removeFromLeft(90));
        if (det) place(*det, knobs.removeFromLeft(90));
    };
    layoutOsc(panelA, dispA, posA, nullptr);
    layoutOsc(panelB, dispB, posB, &detune);

    r.removeFromTop(12);
    auto bottom = r;  // remaining height
    panelMix = bottom.removeFromLeft(110);
    bottom.removeFromLeft(12);
    panelFilter = bottom.removeFromLeft(300);
    bottom.removeFromLeft(12);
    panelEnv = bottom;

    place(mix, panelMix.reduced(10).withTrimmedTop(16));
    auto f = panelFilter.reduced(10).withTrimmedTop(16);
    const int fw = f.getWidth() / 3;
    place(cutoff, f.removeFromLeft(fw));
    place(res, f.removeFromLeft(fw));
    place(drive, f);
    auto e = panelEnv.reduced(10).withTrimmedTop(16);
    const int ew = e.getWidth() / 4;
    place(atk, e.removeFromLeft(ew));
    place(dec, e.removeFromLeft(ew));
    place(sus, e.removeFromLeft(ew));
    place(rel, e);
}

}  // namespace gh
