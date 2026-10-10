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

## Limitations (known)
- Harmonic additive only: no inharmonic partials yet (needed for true bell/metal timbres).
- Filter is a single SVF low-pass; no Virus-style dual-filter routing.
- No mod matrix, LFOs or effects yet.

## Roadmap
1. Compile/fix plugin; add pluginval to CI.
2. Hard sync, FM (A->B) and ring mod between oscillators.
3. Mod matrix + 3 LFOs + filter envelope.
4. Inharmonic partial bank (per-partial frequency ratios) for metallic styles.
5. Effects: distortion, chorus, dubby delay, reverb.
6. Editable additive view (click/drag harmonics), preset save/load, MPE.
7. More generator recipes; "morph between two seeds".
