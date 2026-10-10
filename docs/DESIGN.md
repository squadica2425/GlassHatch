# Design notes

## Sound targets
- **Virus TI character:** two morphing wavetable oscillators, sync/FM/ring mod, drive into a
  resonant filter, fast modulation.
- **Tipper-style downtempo:** heavy sub with a growling mid layer, metallic/glassy textures,
  long dubby space, lots of movement.

## Architecture
```
seed + style -> TableGenerator -> Partials[32 frames x 512 harmonics]
             -> Wavetable::build (IFFT, 10 mip levels) -> TableSet (A, B)
TableSet --(atomic shared_ptr)--> Voice (audio thread)
TableSet --> WavetableDisplay (UI thread)
```
- Phases are fixed per table so frame morphs don't cancel.
- Normalisation uses one gain per frame across all mips, so notes don't jump in level as they
  cross mip boundaries.

## Voice signal flow
```
OscPair (A master -> FM/sync/ring -> B) -> drive (tanh) -> SVF low-pass -> amp ADSR -> out
ModMatrix (every 16 samples): LFO1-3, env, vel, wheel, key, random -> pos/FM/ring/mix/cutoff/reso/drive/pitch
Master: chorus -> ping-pong delay -> reverb
```
Cutoff modulation is exponential (+-4 octaves at amount 1); pitch is +-12 semitones at amount 1.

## Limitations (known)
- Harmonic additive only: no inharmonic partials yet (needed for true bell/metal timbres).
- Hard sync and FM are not oversampled, so extreme settings alias (FM bumps B's mip level by one).
- Filter is a single SVF low-pass; no Virus-style dual-filter routing.
- Modulation runs at control rate (16 samples); LFO rates are free Hz (no tempo sync yet).
- Delay time is in ms (no tempo sync); reverb is `juce::Reverb`.

## Roadmap
1. Compile/fix plugin; add pluginval to CI.
2. Oversampling for sync/FM/drive; second filter with serial/parallel routing and filter envelope.
3. Tempo-synced LFOs/delay; unison/hypersaw mode.
4. Inharmonic partial bank (per-partial frequency ratios) for metallic styles.
5. Editable additive view (click/drag harmonics), preset save/load, MPE.
6. More generator recipes; "morph between two seeds".
