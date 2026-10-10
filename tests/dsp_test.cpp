#include <cstdio>
#include "../src/dsp/Wavetable.h"
#include "../src/gen/TableGenerator.h"
using namespace gh;

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } } while (0)

int main() {
    // 1. IFFT synthesis matches direct summation.
    Partials p; p.amp[0] = 1.f; p.amp[2] = 0.5f; p.phase[2] = 0.7f; p.amp[6] = 0.25f;
    std::vector<float> out(kTableSize);
    renderCycle(p, kMaxPartials, out.data());
    double maxErr = 0;
    for (int n = 0; n < kTableSize; ++n) {
        const double th = 2.0 * 3.14159265358979323846 * n / kTableSize;
        const double ref = std::sin(th) + 0.5 * std::sin(3 * th + 0.7) + 0.25 * std::sin(7 * th);
        maxErr = std::max(maxErr, std::abs(ref - out[n]));
    }
    CHECK(maxErr < 1e-4, "additive IFFT matches direct sum");

    // 2. Mip selection is band-limited: high notes get few harmonics.
    CHECK(Wavetable::mipForIncrement(20.f / 48000.f) == 0, "sub-bass uses full table");
    CHECK(Wavetable::mipForIncrement(8000.f / 48000.f) >= 5, "high note uses a reduced table");

    // 3. Determinism and variety.
    auto a1 = generateTable(42, Style::Glass), a2 = generateTable(42, Style::Glass);
    auto b  = generateTable(43, Style::Glass);
    bool same = true, differ = false;
    for (int h = 0; h < kMaxPartials; ++h) {
        if (a1[5].amp[h] != a2[5].amp[h]) same = false;
        if (a1[5].amp[h] != b[5].amp[h]) differ = true;
    }
    CHECK(same, "same seed -> identical table");
    CHECK(differ, "different seed -> different table");

    // 4. Built tables are normalised and finite for every style.
    for (int s = 0; s < int(Style::Count); ++s) {
        Wavetable wt; wt.build(generateTable(7, Style(s)));
        float peak = 0.f; bool finite = true;
        for (int i = 0; i < 4096; ++i) {
            const float v = wt.read(float(i) / 4096.f, 0.37f, 0);
            if (!std::isfinite(v)) finite = false;
            peak = std::max(peak, std::abs(v));
        }
        CHECK(finite, "table values finite");
        CHECK(peak > 0.5f && peak <= 1.05f, "table normalised");
    }
    std::printf(fails ? "%d test(s) failed\n" : "all DSP tests passed\n", fails);
    return fails;
}
