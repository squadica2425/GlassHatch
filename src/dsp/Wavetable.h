// Band-limited, mip-mapped wavetable built from additive (harmonic) frames.
// JUCE-free so it can be unit-tested on its own.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace gh {

constexpr int   kTableSize   = 2048;  // samples per cycle (power of two)
constexpr int   kMaxPartials = 512;   // harmonics in the top-quality table
constexpr int   kMipLevels   = 10;    // level m keeps kMaxPartials >> m harmonics
constexpr float kPi          = 3.14159265358979f;

// One additive "frame": amplitude + phase for harmonics 1..kMaxPartials.
struct Partials {
    std::array<float, kMaxPartials> amp{};
    std::array<float, kMaxPartials> phase{};
};

namespace detail {
// In-place radix-2 FFT with +i exponent (an unscaled inverse DFT).
inline void fft(std::vector<std::complex<double>>& a) {
    const std::size_t n = a.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (std::size_t len = 2; len <= n; len <<= 1) {
        const double ang = 2.0 * 3.14159265358979323846 / double(len);
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (std::size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (std::size_t k = 0; k < len / 2; ++k) {
                const auto u = a[i + k];
                const auto v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}
}  // namespace detail

// out[n] = sum_h amp[h-1] * sin(2*pi*h*n/N + phase[h-1]), h = 1..maxHarm
inline void renderCycle(const Partials& p, int maxHarm, float* out) {
    std::vector<std::complex<double>> X(kTableSize);
    const int top = std::min({maxHarm, kMaxPartials, kTableSize / 2 - 1});
    for (int h = 1; h <= top; ++h) {
        const double a = p.amp[h - 1], ph = p.phase[h - 1];
        if (a == 0.0) continue;
        X[h] = {a * std::sin(ph), -a * std::cos(ph)};  // Re{X e^{i theta}} = a sin(theta+ph)
    }
    detail::fft(X);
    for (int n = 0; n < kTableSize; ++n) out[n] = float(X[n].real());
}

class Wavetable {
public:
    void build(const std::vector<Partials>& frames) {
        nFrames = int(frames.size());
        data.assign(std::size_t(kMipLevels) * nFrames * stride, 0.0f);
        std::vector<float> tmp(kTableSize);
        for (int f = 0; f < nFrames; ++f) {
            renderCycle(frames[f], kMaxPartials, tmp.data());
            float peak = 0.f;
            for (float v : tmp) peak = std::max(peak, std::abs(v));
            const float gain = peak > 1e-9f ? 1.0f / peak : 0.0f;  // same gain for all mips
            for (int m = 0; m < kMipLevels; ++m) {
                renderCycle(frames[f], kMaxPartials >> m, tmp.data());
                float* dst = at(m, f);
                for (int n = 0; n < kTableSize; ++n) dst[n] = tmp[n] * gain;
                dst[kTableSize] = dst[0];  // guard point for interpolation
            }
        }
    }

    bool empty() const { return nFrames == 0; }
    int  numFrames() const { return nFrames; }

    // phaseInc = freq / sampleRate. Picks the mip whose harmonics stay below Nyquist.
    static int mipForIncrement(float phaseInc) {
        const int allowed = std::max(1, int(0.5f / std::max(phaseInc, 1e-6f)));
        int m = 0;
        while ((kMaxPartials >> m) > allowed && m < kMipLevels - 1) ++m;
        return m;
    }

    // phase in [0,1), framePos in [0,1]
    float read(float phase, float framePos, int mip) const {
        if (nFrames == 0) return 0.f;
        const float fp = std::clamp(framePos, 0.f, 1.f) * float(nFrames - 1);
        const int f0 = int(fp);
        const int f1 = std::min(f0 + 1, nFrames - 1);
        const float ff = fp - float(f0);
        const float a = readFrame(mip, f0, phase);
        const float b = readFrame(mip, f1, phase);
        return a + (b - a) * ff;
    }

    // Raw access for UI drawing (mip 0, one sample of one frame).
    float sampleAt(int frame, int index) const { return at(0, frame)[index & (kTableSize - 1)]; }

private:
    static constexpr std::size_t stride = kTableSize + 1;
    float* at(int m, int f) { return data.data() + (std::size_t(m) * nFrames + f) * stride; }
    const float* at(int m, int f) const { return data.data() + (std::size_t(m) * nFrames + f) * stride; }
    float readFrame(int m, int f, float phase) const {
        const float x = phase * float(kTableSize);
        int i = int(x);
        if (i >= kTableSize) i = kTableSize - 1;
        const float fr = x - float(i);
        const float* t = at(m, f);
        return t[i] + (t[i + 1] - t[i]) * fr;
    }
    int nFrames = 0;
    std::vector<float> data;
};

}  // namespace gh
