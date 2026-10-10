# GlassHatch

An open-source VST3 synth: **dual wavetable oscillators built by additive synthesis**, with
**endless seeded sound generation**. Inspired by the Access Virus TI's morphing wavetables and
the dense, glassy, sub-heavy downtempo palette of artists like Tipper. No samples or factory
data from either are used; everything is generated procedurally.

## How it works
- **Additive tables.** Each wavetable frame is a set of harmonic amplitudes/phases rendered with
  an inverse FFT (`src/dsp/Wavetable.h`). Tables are mip-mapped and band-limited, so high notes
  don't alias.
- **Infinite generation.** `(seed, style)` deterministically produces a 32-frame table per
  oscillator (`src/gen/TableGenerator.h`). Styles: Glass, Sub Growl, Metallic, Vocal. Hit
  **Randomize** or step the seed; tables are rebuilt on a background thread and swapped in
  atomically, so the audio thread never blocks.
- **UI.** Serum/Current-style: each oscillator shows its full 3D frame stack with the current
  frame highlighted, plus the harmonic spectrum of the interpolated frame. Drag across the view to
  scrub the position.
- **Oscillator interaction.** Hard sync (B restarts when A wraps, sub-sample accurate), FM (A -> B
  phase modulation) and ring mod (crossfade to A x B).
- **Modulation.** 3 LFOs (sine/tri/saw/square/S&H) and a 6-slot matrix. Sources: LFO 1-3, amp env,
  velocity, mod wheel, key, per-note random. Destinations: pos A/B, FM, ring, mix, cutoff, reso, drive, pitch.
- **Effects.** Chorus, dubby ping-pong delay (darkening feedback, gliding time), reverb.
- **Voice.** 8 voices, drive into a resonant SVF low-pass, ADSR.

## Build
```bash
# DSP unit tests only (no dependencies)
cmake -B build -DGH_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build

# Plugin (fetches JUCE 8)
cmake -B build && cmake --build build --config Release
# VST3 ends up in build/GlassHatch_artefacts/Release/VST3
```

## Status
- DSP core, generator, sync/FM/ring, LFOs, matrix, chorus/delay and tests: written and passing (`ctest`).
- JUCE plugin and UI: written but not yet compiled; expect small fixes on first build.

## Licensing
JUCE is AGPLv3 or commercial. Pick a compatible license (AGPLv3 is the simplest) before
publishing, and add a `LICENSE` file.

## Roadmap
See `docs/DESIGN.md`.
