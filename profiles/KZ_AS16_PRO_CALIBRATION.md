# KZ AS16 Pro — calibration development

## Identity
- Model: KZ AS16 Pro (original, NOT AS16 Pro X, NOT High-IMP edition).
- Ear tips: stock silicone.
- Artwork: supplied photo; cinematic dark version created separately.
- Existing Samson SR850 profile must remain bit-identical to release #17.

## Measurement sources found
1. Reference Audio Analyzer — KZ AS16 Pro (IEC711 and SIEC measurements):
   https://reference-audio-analyzer.pro/en/report/hp/kz-as16-pro.php
   https://reference-audio-analyzer.pro/en/report-pro-hp-target.php?id=5364
   Note: numeric CSV export appears to require Pro account. Do not digitize screenshots as ground truth.
2. EPLV Squiglink — includes KZ AS16 Pro, KZ AS16 Pro +22 Ohm, KZ AS16 Pro High-IMP Edition, KZ AS16 Pro X. **Use only the unmodified KZ AS16 Pro**:
   https://eplv.squig.link/
3. boizoff Squiglink — KZ AS16 Pro:
   https://boizoff.squig.link/

## Calibration gate (do not skip)
- Obtain at least one actual numerical frequency-response dataset and its corresponding target curve from the same measurement rig.
- Prefer multiple independently measured datasets, but never average unaligned rigs/targets.
- Normalize at 500–1000 Hz, use broad-band smoothing, constrain narrow treble peaks and limit boosts.
- Apply conservative correction above 8–10 kHz because IEC711 canal resonances are fit-dependent.
- Export fixed minimum-phase FIR with reproducible sample rate, length and checksum.
- Test correction frequency response numerically and verify at 44.1/48/96 kHz.
- Add user-selectable headphone profile and image switch only once both FIRs are valid.
- Crossfade when changing convolution profiles to avoid clicks; preserve host automation and plugin state.
- The spectrum analyzer must show post-correction audio and fixed EQ for the selected profile.

## Status
Branch created for multi-headphone development. **KZ correction is NOT calibrated or compiled yet**. No speculative EQ values should be released.
