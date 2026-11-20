#!/usr/bin/env python3
"""Build a conservative KZ AS16 Pro FIR from numerical measurements.

Requires measured FR and target measured on the SAME coupler/rig.
CSV columns: frequency,raw (or frequency,db); target: frequency,target (or frequency,db).
No invented response data or automatic cross-rig averaging.
"""
import argparse
import csv
from pathlib import Path
import numpy as np
from scipy.signal import minimum_phase
from scipy.io.wavfile import write

def read_curve(path, aliases):
    with open(path, newline="", encoding="utf-8-sig") as fh:
        reader = csv.DictReader(fh)
        if not reader.fieldnames:
            raise ValueError(f"{path}: missing CSV header")
        cols = {name.strip().lower(): name for name in reader.fieldnames}
        fcol = cols.get("frequency", cols.get("freq", cols.get("hz")))
        dcol = next((cols[x] for x in aliases if x in cols), None)
        if not fcol or not dcol:
            raise ValueError(f"{path}: expected frequency and one of {aliases}")
        data = [(float(row[fcol]),float(row[dcol])) for row in reader]
    data = np.array(data, dtype=np.float64)
    if data.ndim != 2 or data.shape[0] < 80:
        raise ValueError(f"{path}: require >=80 numeric frequency points")
    if not np.isfinite(data).all() or np.any(data[:,0] <= 0):
        raise ValueError(f"{path}: non-finite data or invalid frequency")
    data = data[np.argsort(data[:,0])]
    if np.any(np.diff(data[:,0]) <= 0):
        raise ValueError(f"{path}: repeated frequency")
    if data[0,0] > 30 or data[-1,0] < 10000:
        raise ValueError(f"{path}: inadequate measured bandwidth")
    return data[:,0],data[:,1]

def interp_log(freq, db, x):
    return np.interp(np.log(np.maximum(x,freq[0])),np.log(freq),db)

def smooth_log(freq, db, octave=1/6):
    out = np.empty_like(db)
    for i,f in enumerate(freq):
        mask = np.abs(np.log2(freq/f)) <= octave/2
        out[i] = np.mean(db[mask]) if mask.any() else db[i]
    return out

def build(measurement, target, output, rate=48000, taps=8192):
    mf,md=read_curve(measurement,("raw","db","spl","level"))
    tf,td=read_curve(target,("target","db","spl","level"))
    # Match level in the 500-1000 Hz reference region.
    f=np.geomspace(20,min(20000,rate*.45),1200)
    m=interp_log(mf,md,f)
    t=interp_log(tf,td,f)
    reference=(f>=500)&(f<=1000)
    delta=smooth_log(f,t-m)
    delta-=np.mean(delta[reference])
    # Do not chase unreliable coupler resonances >8 kHz.
    attenuation=np.clip((np.log2(np.maximum(f,8000)/8000))/np.log2(14000/8000),0,1)
    delta *= (1-attenuation)
    # Restrict boosts and narrow notches; retain broad cuts.
    delta=np.clip(delta,-9,4)
    fft_size=32768
    bins=np.fft.rfftfreq(fft_size,1/rate)
    db=interp_log(f,delta,np.maximum(bins,20))
    db[bins>14000]=0
    spectrum=10**(db/20)
    prototype=np.fft.irfft(spectrum,n=fft_size)
    prototype=np.roll(prototype,fft_size//2)
    impulse=minimum_phase(prototype,method="homomorphic",half=False)
    impulse=np.asarray(impulse[:taps],dtype=np.float64)
    # Correct magnitude of the windowed impulse against desired at 1 kHz.
    response=np.fft.rfft(impulse,fft_size)
    k=np.argmin(abs(bins-1000))
    impulse/=max(abs(response[k]),1e-8)
    output=Path(output)
    output.parent.mkdir(parents=True,exist_ok=True)
    write(output,rate,impulse.astype(np.float32))
    print(f"Saved {output}; {taps} taps; {rate} Hz; peak boost <=4 dB; correction tapers to 0 above 14 kHz")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--measurement",required=True)
    p.add_argument("--target",required=True)
    p.add_argument("--output",required=True)
    p.add_argument("--rate",type=int,default=48000)
    p.add_argument("--taps",type=int,default=8192)
    a=p.parse_args()
    build(a.measurement,a.target,a.output,a.rate,a.taps)
