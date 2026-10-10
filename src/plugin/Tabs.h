// Lower half of the editor: SOUND (osc interaction / filter / env), MOD (LFOs + matrix), FX.
#pragma once
#include "../dsp/Lfo.h"
#include "../dsp/ModMatrix.h"
#include "Knob.h"

namespace gh {

inline const juce::Colour kCyan{0xff4fd1ff}, kPink{0xffff5fa8}, kOrange{0xffffb347}, kViolet{0xffb48cff}, kGreen{0xff7dffb0};

// ------------------------------------------------------------------ SOUND
class SoundTab : public juce::Component {
public:
    explicit SoundTab(APVTS& a) {
        mix.init(*this, a, "mix", "A / B", kOrange);
        fm.init(*this, a, "fm", "FM A>B", kOrange);
        ring.init(*this, a, "ring", "Ring", kOrange);
        cutoff.init(*this, a, "cutoff", "Cutoff", kGreen);
        res.init(*this, a, "res", "Reso", kGreen);
        drive.init(*this, a, "drive", "Drive", kGreen);
        atk.init(*this, a, "atk", "A", kViolet);
        dec.init(*this, a, "dec", "D", kViolet);
        sus.init(*this, a, "sus", "S", kViolet);
        rel.init(*this, a, "rel", "R", kViolet);
        sync.setButtonText("Sync");
        sync.setClickingTogglesState(true);
        sync.setColour(juce::TextButton::buttonOnColourId, kOrange);
        addAndMakeVisible(sync);
        syncAtt = std::make_unique<APVTS::ButtonAttachment>(a, "sync", sync);
    }
    void paint(juce::Graphics& g) override {
        drawPanel(g, pOsc, "OSC INTERACTION", kOrange);
        drawPanel(g, pFilter, "FILTER", kGreen);
        drawPanel(g, pEnv, "AMP ENVELOPE", kViolet);
    }
    void resized() override {
        auto r = getLocalBounds();
        pOsc = r.removeFromLeft(340); r.removeFromLeft(12);
        pFilter = r.removeFromLeft(260); r.removeFromLeft(12);
        pEnv = r;
        auto o = pOsc.reduced(10).withTrimmedTop(16);
        const int ow = o.getWidth() / 4;
        mix.place(o.removeFromLeft(ow));
        fm.place(o.removeFromLeft(ow));
        ring.place(o.removeFromLeft(ow));
        sync.setBounds(o.reduced(4, 0).withSizeKeepingCentre(ow - 8, 30));
        auto f = pFilter.reduced(10).withTrimmedTop(16);
        const int fw = f.getWidth() / 3;
        cutoff.place(f.removeFromLeft(fw)); res.place(f.removeFromLeft(fw)); drive.place(f);
        auto e = pEnv.reduced(10).withTrimmedTop(16);
        const int ew = e.getWidth() / 4;
        atk.place(e.removeFromLeft(ew)); dec.place(e.removeFromLeft(ew)); sus.place(e.removeFromLeft(ew)); rel.place(e);
    }
private:
    Knob mix, fm, ring, cutoff, res, drive, atk, dec, sus, rel;
    juce::TextButton sync;
    std::unique_ptr<APVTS::ButtonAttachment> syncAtt;
    juce::Rectangle<int> pOsc, pFilter, pEnv;
};

// -------------------------------------------------------------------- MOD
class ModTab : public juce::Component {
public:
    explicit ModTab(APVTS& a) {
        juce::StringArray shapes, srcs, dsts;
        for (auto* n : kLfoShapeNames) shapes.add(n);
        for (auto* n : kSrcNames) srcs.add(n);
        for (auto* n : kDstNames) dsts.add(n);
        for (int i = 0; i < 3; ++i) {
            const auto id = "lfo" + juce::String(i + 1);
            lfoShape[i].init(*this, a, id + "Shape", shapes);
            lfoRate[i].setSliderStyle(juce::Slider::LinearHorizontal);
            lfoRate[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
            lfoRate[i].setTextValueSuffix(" Hz");
            lfoRate[i].setColour(juce::Slider::trackColourId, kViolet);
            addAndMakeVisible(lfoRate[i]);
            lfoRateAtt[i] = std::make_unique<APVTS::SliderAttachment>(a, id + "Rate", lfoRate[i]);
            lfoName[i].setText("LFO " + juce::String(i + 1), juce::dontSendNotification);
            lfoName[i].setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
            addAndMakeVisible(lfoName[i]);
        }
        for (int i = 0; i < kNumSlots; ++i) {
            const auto id = "m" + juce::String(i + 1);
            src[i].init(*this, a, id + "Src", srcs);
            dst[i].init(*this, a, id + "Dst", dsts);
            amt[i].setSliderStyle(juce::Slider::LinearHorizontal);
            amt[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
            amt[i].setColour(juce::Slider::trackColourId, kOrange);
            addAndMakeVisible(amt[i]);
            amtAtt[i] = std::make_unique<APVTS::SliderAttachment>(a, id + "Amt", amt[i]);
        }
    }
    void paint(juce::Graphics& g) override {
        drawPanel(g, pLfo, "LFOs", kViolet);
        drawPanel(g, pMat, "MOD MATRIX   (source  >  destination  x  amount)", kOrange);
    }
    void resized() override {
        auto r = getLocalBounds();
        pLfo = r.removeFromLeft(300); r.removeFromLeft(12);
        pMat = r;
        auto l = pLfo.reduced(10).withTrimmedTop(20);
        const int lh = l.getHeight() / 3;
        for (int i = 0; i < 3; ++i) {
            auto row = l.removeFromTop(lh).reduced(0, 4);
            lfoName[i].setBounds(row.removeFromLeft(48));
            lfoShape[i].box.setBounds(row.removeFromLeft(84).reduced(0, 3));
            row.removeFromLeft(6);
            lfoRate[i].setBounds(row);
        }
        auto m = pMat.reduced(10).withTrimmedTop(20);
        const int mh = m.getHeight() / kNumSlots;
        for (int i = 0; i < kNumSlots; ++i) {
            auto row = m.removeFromTop(mh).reduced(0, 3);
            src[i].box.setBounds(row.removeFromLeft(120)); row.removeFromLeft(8);
            dst[i].box.setBounds(row.removeFromLeft(120)); row.removeFromLeft(8);
            amt[i].setBounds(row);
        }
    }
private:
    ChoiceBox lfoShape[3], src[kNumSlots], dst[kNumSlots];
    juce::Slider lfoRate[3], amt[kNumSlots];
    juce::Label lfoName[3];
    std::unique_ptr<APVTS::SliderAttachment> lfoRateAtt[3], amtAtt[kNumSlots];
    juce::Rectangle<int> pLfo, pMat;
};

// --------------------------------------------------------------------- FX
class FxTab : public juce::Component {
public:
    explicit FxTab(APVTS& a) {
        chRate.init(*this, a, "chRate", "Rate", kCyan);
        chDepth.init(*this, a, "chDepth", "Depth", kCyan);
        chMix.init(*this, a, "chMix", "Mix", kCyan);
        dlTime.init(*this, a, "dlTime", "Time ms", kPink);
        dlFb.init(*this, a, "dlFb", "Feedback", kPink);
        dlMix.init(*this, a, "dlMix", "Mix", kPink);
        rvSize.init(*this, a, "rvSize", "Size", kViolet);
        rvMix.init(*this, a, "rvMix", "Mix", kViolet);
    }
    void paint(juce::Graphics& g) override {
        drawPanel(g, pCh, "CHORUS", kCyan);
        drawPanel(g, pDl, "PING-PONG DELAY", kPink);
        drawPanel(g, pRv, "REVERB", kViolet);
    }
    void resized() override {
        auto r = getLocalBounds();
        const int w = (r.getWidth() - 24) / 3;
        pCh = r.removeFromLeft(w); r.removeFromLeft(12);
        pDl = r.removeFromLeft(w); r.removeFromLeft(12);
        pRv = r;
        auto row3 = [](juce::Rectangle<int> p, Knob& a, Knob& b, Knob& c) {
            auto q = p.reduced(10).withTrimmedTop(16);
            const int cw = q.getWidth() / 3;
            a.place(q.removeFromLeft(cw)); b.place(q.removeFromLeft(cw)); c.place(q);
        };
        row3(pCh, chRate, chDepth, chMix);
        row3(pDl, dlTime, dlFb, dlMix);
        auto q = pRv.reduced(10).withTrimmedTop(16);
        const int cw = q.getWidth() / 2;
        rvSize.place(q.removeFromLeft(cw)); rvMix.place(q);
    }
private:
    Knob chRate, chDepth, chMix, dlTime, dlFb, dlMix, rvSize, rvMix;
    juce::Rectangle<int> pCh, pDl, pRv;
};

}  // namespace gh
