#include "PluginEditor.h"
#include "Tabs.h"

namespace gh {

GlassHatchEditor::GlassHatchEditor(GlassHatchProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p),
      dispA(p, false, "posA", kCyan), dispB(p, true, "posB", kPink) {
    setSize(900, 760);

    addAndMakeVisible(dispA);
    addAndMakeVisible(dispB);
    posA.init(*this, p.apvts, "posA", "Position", kCyan);
    posB.init(*this, p.apvts, "posB", "Position", kPink);
    detune.init(*this, p.apvts, "detune", "Detune", kPink);

    title.setText("GLASSHATCH", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(title);

    styleBox.addItemList({"Glass", "Sub Growl", "Metallic", "Vocal"}, 1);
    addAndMakeVisible(styleBox);
    styleAtt = std::make_unique<APVTS::ComboBoxAttachment>(proc.apvts, "style", styleBox);

    seedSlider.setSliderStyle(juce::Slider::IncDecButtons);
    seedSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 24);
    addAndMakeVisible(seedSlider);
    seedAtt = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "seed", seedSlider);

    dice.onClick = [this] {
        if (auto* s = proc.apvts.getParameter("seed")) {
            s->beginChangeGesture();
            s->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
            s->endChangeGesture();
        }
    };
    addAndMakeVisible(dice);

    const juce::Colour tabBg(0xff0b0d14);
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);
    tabs.setIndent(0);
    tabs.addTab("SOUND", tabBg, new SoundTab(proc.apvts), true);
    tabs.addTab("MOD", tabBg, new ModTab(proc.apvts), true);
    tabs.addTab("FX", tabBg, new FxTab(proc.apvts), true);
    addAndMakeVisible(tabs);
}

void GlassHatchEditor::paint(juce::Graphics& g) {
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff151826), 0.f, 0.f,
                                           juce::Colour(0xff07080d), 0.f, float(getHeight()), false));
    g.fillAll();
    drawPanel(g, panelA, "OSC A", kCyan);
    drawPanel(g, panelB, "OSC B", kPink);
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

    auto oscRow = r.removeFromTop(340);
    const int half = (oscRow.getWidth() - 12) / 2;
    panelA = oscRow.removeFromLeft(half);
    oscRow.removeFromLeft(12);
    panelB = oscRow;

    auto layoutOsc = [](juce::Rectangle<int> pnl, WavetableDisplay& d, Knob& pos, Knob* det) {
        auto inner = pnl.reduced(10);
        inner.removeFromTop(16);
        auto knobs = inner.removeFromBottom(92);
        d.setBounds(inner);
        pos.place(knobs.removeFromLeft(90));
        if (det) det->place(knobs.removeFromLeft(90));
    };
    layoutOsc(panelA, dispA, posA, nullptr);
    layoutOsc(panelB, dispB, posB, &detune);

    r.removeFromTop(12);
    tabs.setBounds(r);
}

}  // namespace gh
