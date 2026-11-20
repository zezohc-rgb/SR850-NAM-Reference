"""Build and verify an exact minimum-phase FIR from the published AutoEq curve.

The frequency response is reconstructed by real-cepstrum spectral factorisation,
rather than using a heuristic FIR minimum-phase conversion.
"""
import csv
import io
import pathlib
import sys
import urllib.request

import numpy as np
from scipy.io import wavfile

URL = "https://raw.githubusercontent.com/jaakkopasanen/AutoEq/master/results/Rtings/HMS%20II.3%20over-ear/Samson%20SR850/Samson%20SR850.csv"
dest = pathlib.Path(sys.argv[1])
with urllib.request.urlopen(URL, timeout=40) as response:
    rows = list(csv.DictReader(io.StringIO(response.read().decode("utf-8-sig"))))
if len(rows) < 600:
    raise RuntimeError("Incomplete SR850 curve")

freq = np.asarray([float(r["frequency"]) for r in rows], dtype=np.float64)
db = np.asarray([float(r["equalization"]) for r in rows], dtype=np.float64)
if not (np.all(np.isfinite(freq)) and np.all(np.isfinite(db)) and np.all(np.diff(freq) > 0)):
    raise RuntimeError("Invalid AutoEq frequency data")

sr = int(sys.argv[2]) if len(sys.argv) > 2 else 48000
if sr not in (44100, 48000):
    raise ValueError("Supported sample rates: 44100, 48000")
nfft = 16384
bins = np.fft.rfftfreq(nfft, 1 / sr)
target_db = np.interp(np.log10(np.maximum(bins, freq[0])), np.log10(freq), db)
target_db[bins > freq[-1]] = db[-1]

# Differential transfer measured from Morphit.wav and Nosso VST.wav
# (same 16-second passage at 44.1 kHz, stereo). Broad anchor points
# avoid overfitting individual narrow peaks or high-frequency noise.
# This deliberately targets the Morphit tonal balance, not a proven flat target.
reference_hz = np.array([
    20, 40, 65, 80, 100, 130, 160, 200, 250, 320, 400, 500,
    650, 800, 1000, 1250, 1600, 2000, 2500, 3000, 3500, 4000,
    5000, 6000, 7000, 8000, 9000, 10000, 11000, 12500,
    14500, 16500, 18000, 20000, 24000
], dtype=np.float64)
reference_db = np.array([
    0, -1.5, -0.3, 0.2, 0.5, 1.35, 1.9, 2.1, 1.8, 1.5, 0.8, 0.0,
    -0.65, -0.3, 0.45, 0.3, -0.45, -0.3, -0.45, 0.2, 0.5, 0.0,
    0.6, 1.3, 0.8, 0.4, 1.3, 2.7, 5.0, 6.2,
    6.3, 6.4, 6.0, 3.0, 0.0
], dtype=np.float64)
target_db += np.interp(
    np.log10(np.maximum(bins, reference_hz[0])),
    np.log10(reference_hz), reference_db
)


# Real cepstrum: double the positive quefrencies and discard negative ones.
# This reconstructs a causal minimum-phase spectrum with the requested magnitude.
log_magnitude = target_db * np.log(10.0) / 20.0
cepstrum = np.fft.irfft(log_magnitude, nfft)
minimum_cepstrum = np.zeros(nfft, dtype=np.float64)
minimum_cepstrum[0] = cepstrum[0]
minimum_cepstrum[1:nfft // 2] = 2.0 * cepstrum[1:nfft // 2]
minimum_cepstrum[nfft // 2] = cepstrum[nfft // 2]
minimum_spectrum = np.exp(np.fft.rfft(minimum_cepstrum))
impulse = np.fft.irfft(minimum_spectrum, nfft)

# Check the exact stored float32 response against the intended target curve.
stored = impulse.astype(np.float32)
actual_db = 20.0 * np.log10(np.maximum(np.abs(np.fft.rfft(stored)), 1e-12))
max_error_db = float(np.max(np.abs(actual_db - target_db)))
if max_error_db > 0.05:
    raise RuntimeError(f"FIR correction differs from target by {max_error_db:.3f} dB")
if not np.all(np.isfinite(stored)):
    raise RuntimeError("Invalid FIR samples")

dest.parent.mkdir(parents=True, exist_ok=True)
wavfile.write(dest, sr, stored)
print(f"Generated {dest} from {len(rows)} points; maximum FFT-bin magnitude error: {max_error_db:.6f} dB")
