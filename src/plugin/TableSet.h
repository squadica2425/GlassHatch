#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "../dsp/Wavetable.h"
#include "../gen/TableGenerator.h"

namespace gh {

// Everything one patch needs: playback tables for both oscillators plus the
// additive source frames (kept so the UI can draw the spectrum).
struct TableSet {
    Wavetable wtA, wtB;
    std::vector<Partials> framesA, framesB;

    static std::shared_ptr<const TableSet> make(std::uint32_t seed, Style style) {
        auto t = std::make_shared<TableSet>();
        t->framesA = generateTable(seed, style);
        t->framesB = generateTable(seed ^ 0x9E3779B9u, style);
        t->wtA.build(t->framesA);
        t->wtB.build(t->framesB);
        return t;
    }
};

}  // namespace gh
