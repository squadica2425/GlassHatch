// 6-slot modulation matrix: (source, destination, bipolar amount).
#pragma once

namespace gh {

enum ModSrc { SrcNone = 0, SrcLfo1, SrcLfo2, SrcLfo3, SrcEnv, SrcVel, SrcWheel, SrcKey, SrcRandom, kNumSrc };
enum ModDst { DstNone = 0, DstPosA, DstPosB, DstFm, DstRing, DstMix, DstCutoff, DstReso, DstDrive, DstPitch, kNumDst };

constexpr const char* kSrcNames[kNumSrc] = {"None", "LFO 1", "LFO 2", "LFO 3", "Amp Env",
                                            "Velocity", "Mod Wheel", "Key", "Random"};
constexpr const char* kDstNames[kNumDst] = {"None", "Osc A Pos", "Osc B Pos", "FM", "Ring",
                                            "Mix", "Cutoff", "Reso", "Drive", "Pitch"};
constexpr int kNumSlots = 6;

struct ModSlot { int src = 0, dst = 0; float amt = 0.f; };

// src[]: current source values (LFOs/key/random -1..1, env/vel/wheel 0..1).
// dstSum[]: summed modulation per destination, in "full-scale = 1" units.
inline void applyMatrix(const ModSlot* slots, int n, const float* src, float* dstSum) {
    for (int d = 0; d < kNumDst; ++d) dstSum[d] = 0.f;
    for (int i = 0; i < n; ++i)
        if (slots[i].src > 0 && slots[i].src < kNumSrc && slots[i].dst > 0 && slots[i].dst < kNumDst)
            dstSum[slots[i].dst] += src[slots[i].src] * slots[i].amt;
}

}  // namespace gh
