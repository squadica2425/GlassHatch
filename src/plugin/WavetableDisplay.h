// Serum/Current-style wavetable view:
//  - top: 3D stack of every frame (front = frame 0), current frame highlighted
//  - bottom: harmonic spectrum of the interpolated frame (the additive source)
//  - drag horizontally to scrub the wavetable position
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace gh {

class WavetableDisplay : public juce::Component, private juce::Timer {
public:
    WavetableDisplay(GlassHatchProcessor& p, bool isB, const juce::String& posId, juce::Colour c)
        : proc(p), useB(isB), colour(c), param(p.apvts.getParameter(posId)) {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff0b0d14));
        g.fillRoundedRectangle(bounds, 6.f);
        g.setColour(colour.withAlpha(0.25f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.f, 1.f);

        auto tables = proc.getTables();
        if (!tables) return;
        const auto& wt = useB ? tables->wtB : tables->wtA;
        const auto& frames = useB ? tables->framesB : tables->framesA;
        const int nF = wt.numFrames();
        if (nF < 2) return;
        if (tables != cachedTables) { rebuildPaths(wt, nF); cachedTables = tables; }

        auto area = bounds.reduced(12.f);
        auto specArea = area.removeFromBottom(area.getHeight() * 0.26f);
        area.removeFromBottom(6.f);

        // ---- 3D stack ----
        const float waveW = area.getWidth() * 0.72f;
        const float waveH = area.getHeight() * 0.20f;  // half-height of one wave
        const float stepX = (area.getWidth() - waveW) / float(nF - 1);
        const float stepY = (area.getHeight() - 2.f * waveH) / float(nF - 1);
        const float pos = float(param->getValue());
        const int active = juce::jlimit(0, nF - 1, int(std::round(pos * float(nF - 1))));

        auto place = [&](int f) {
            return juce::AffineTransform::scale(waveW, waveH)
                .translated(area.getX() + stepX * float(f), area.getBottom() - waveH - stepY * float(f));
        };
        for (int f = nF - 1; f >= 0; --f) {          // back to front
            if (f == active) continue;
            const float depth = 1.f - float(f) / float(nF);
            g.setColour(colour.withAlpha(0.10f + 0.35f * depth));
            g.strokePath(paths[size_t(f)], juce::PathStrokeType(1.f), place(f));
        }
        g.setColour(colour.withAlpha(0.25f));
        g.strokePath(paths[size_t(active)], juce::PathStrokeType(6.f), place(active));  // glow
        g.setColour(juce::Colours::white);
        g.strokePath(paths[size_t(active)], juce::PathStrokeType(1.8f), place(active));

        // ---- spectrum of the interpolated frame ----
        const float fp = pos * float(nF - 1);
        const int f0 = int(fp), f1 = std::min(f0 + 1, nF - 1);
        const float t = fp - float(f0);
        constexpr int bars = 64;
        float amps[bars], peak = 1e-9f;
        for (int h = 0; h < bars; ++h) {
            amps[h] = frames[size_t(f0)].amp[size_t(h)] * (1.f - t) + frames[size_t(f1)].amp[size_t(h)] * t;
            peak = std::max(peak, amps[h]);
        }
        const float bw = specArea.getWidth() / float(bars);
        for (int h = 0; h < bars; ++h) {
            const float v = amps[h] / peak;
            const float bh = std::sqrt(v) * specArea.getHeight();  // sqrt = gentle compression
            g.setColour(colour.withAlpha(0.35f + 0.55f * v));
            g.fillRect(specArea.getX() + bw * float(h) + 0.5f, specArea.getBottom() - bh, bw - 1.f, bh);
        }
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.setFont(11.f);
        g.drawText(useB ? "OSC B" : "OSC A", bounds.reduced(10.f, 6.f), juce::Justification::topLeft);
        g.drawText("frame " + juce::String(active + 1) + "/" + juce::String(nF),
                   bounds.reduced(10.f, 6.f), juce::Justification::topRight);
    }

    void mouseDown(const juce::MouseEvent& e) override { param->beginChangeGesture(); setFromMouse(e); }
    void mouseDrag(const juce::MouseEvent& e) override { setFromMouse(e); }
    void mouseUp(const juce::MouseEvent&) override { param->endChangeGesture(); }

private:
    void setFromMouse(const juce::MouseEvent& e) {
        const float v = juce::jlimit(0.f, 1.f, (float(e.x) - 12.f) / float(getWidth() - 24));
        param->setValueNotifyingHost(v);
    }
    void timerCallback() override { repaint(); }

    void rebuildPaths(const Wavetable& wt, int nF) {
        constexpr int pts = 160;
        paths.assign(size_t(nF), {});
        for (int f = 0; f < nF; ++f) {
            auto& p = paths[size_t(f)];
            for (int i = 0; i <= pts; ++i) {
                const float x = float(i) / float(pts);
                const float y = -wt.sampleAt(f, (i * kTableSize) / pts);  // y grows downward
                if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
            }
        }
    }

    GlassHatchProcessor& proc;
    bool useB;
    juce::Colour colour;
    juce::RangedAudioParameter* param;
    std::shared_ptr<const TableSet> cachedTables;  // holding a ref avoids stale-pointer reuse
    std::vector<juce::Path> paths;  // unit space: x 0..1, y -1..1
};

}  // namespace gh
