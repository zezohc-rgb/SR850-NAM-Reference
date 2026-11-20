"""Generate minimum-phase FIR from published AutoEq SR850 full curve."""
import csv, io, pathlib, sys, urllib.request
import numpy as np
from scipy.signal import minimum_phase
from scipy.io import wavfile
URL = "https://raw.githubusercontent.com/jaakkopasanen/AutoEq/master/results/Rtings/HMS%20II.3%20over-ear/Samson%20SR850/Samson%20SR850.csv"
dest = pathlib.Path(sys.argv[1])
with urllib.request.urlopen(URL, timeout=40) as response:
    rows = list(csv.DictReader(io.StringIO(response.read().decode("utf-8-sig"))))
if len(rows) < 600: raise RuntimeError("Incomplete SR850 curve")
freq = np.array([float(r["frequency"]) for r in rows])
db = np.array([float(r["equalization"]) for r in rows])
sr, nfft = 48000, 16384
bins = np.fft.rfftfreq(nfft, 1/sr)
interp = np.interp(np.log10(np.maximum(bins, freq[0])), np.log10(freq), db)
interp[bins > freq[-1]] = db[-1]
mag = 10**(interp/20)
linear = np.roll(np.fft.irfft(mag, nfft), nfft//2)
minimum = minimum_phase(linear, method="homomorphic", half=False)
dest.parent.mkdir(parents=True, exist_ok=True)
wavfile.write(dest, sr, minimum.astype(np.float32))
print(f"Generated {dest} from {len(rows)} frequency points")
