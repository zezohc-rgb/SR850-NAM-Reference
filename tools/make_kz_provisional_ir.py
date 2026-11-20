#!/usr/bin/env python3
"""Evidence-based KZ AS16 Pro FIR (provisional).
Source: https://www.speakerranking.com/headphones/models/KZ-AS16-Pro/
Neutral target preset derived from median of 13 independent Squiglink measurements.
This reproduces published PEQ as a minimum-phase FIR; NOT a direct raw-FR inversion.
"""
import sys
import numpy as np
from scipy.io.wavfile import write
from pathlib import Path

RATE=48000
FFT=65536
TAPS=16384
# Published neutral target: low shelf, high shelf, eight peaking filters.
FILTERS=[
 ("low",105,-7.0,.70),
 ("high",10000,1.4,.70),
 ("peak",82,-1.0,1.39),
 ("peak",424,4.0,1.68),
 ("peak",613,1.6,4.09),
 ("peak",1153,-3.8,2.85),
 ("peak",2116,1.2,5.42),
 ("peak",3030,-2.0,6.0),
 ("peak",4321,6.0,1.44),
 ("peak",5744,2.9,6.0),
]
def coeff(kind,hz,db,q):
    A=10**(db/40)
    w=2*np.pi*hz/RATE
    c=np.cos(w);sn=np.sin(w)
    if kind=="peak":
        alpha=sn/(2*q)
        b=[1+alpha*A,-2*c,1-alpha*A]
        a=[1+alpha/A,-2*c,1-alpha/A]
    else:
        # RBJ cookbook shelf: Q parameter is shelf slope S, as used in AutoEQ filters.
        S=q
        alpha=sn/2*np.sqrt((A+1/A)*(1/S-1)+2)
        beta=2*np.sqrt(A)*alpha
        if kind=="low":
            b=[A*((A+1)-(A-1)*c+beta),2*A*((A-1)-(A+1)*c),A*((A+1)-(A-1)*c-beta)]
            a=[(A+1)+(A-1)*c+beta,-2*((A-1)+(A+1)*c),(A+1)+(A-1)*c-beta]
        else:
            b=[A*((A+1)+(A-1)*c+beta),-2*A*((A-1)+(A+1)*c),A*((A+1)+(A-1)*c-beta)]
            a=[(A+1)-(A-1)*c+beta,2*((A-1)-(A+1)*c),(A+1)-(A-1)*c-beta]
    return np.array(b)/a[0],np.array(a)/a[0]

freq=np.fft.rfftfreq(FFT,1/RATE)
z=np.exp(-2j*np.pi*freq/RATE)
response=np.ones_like(z,dtype=complex)
for kind,hz,db,q in FILTERS:
    b,a=coeff(kind,hz,db,q)
    response*= (b[0]+b[1]*z+b[2]*z*z)/(a[0]+a[1]*z+a[2]*z*z)
# Use exact RBJ response magnitude and reconstruct minimum phase via real cepstrum.
logmag=np.log(np.maximum(abs(response),1e-10))
cep=np.fft.irfft(logmag,n=FFT)
mincep=np.zeros(FFT)
mincep[0]=cep[0]
mincep[1:FFT//2]=2*cep[1:FFT//2]
mincep[FFT//2]=cep[FFT//2]
spectrum=np.exp(np.fft.rfft(mincep))
impulse=np.fft.irfft(spectrum,n=FFT)[:TAPS]
# Reserve headroom for published 6 dB boost; gain parameter stays available in plugin.
impulse*=10**(-6.4/20)
output=Path(sys.argv[1])
output.parent.mkdir(parents=True,exist_ok=True)
write(output,RATE,impulse.astype(np.float32))
actual=np.fft.rfft(impulse,n=FFT)
error=np.max(np.abs(20*np.log10(np.maximum(abs(actual),1e-10))-20*np.log10(np.maximum(abs(response),1e-10))+6.4))
print(f"KZ provisional FIR: {TAPS} taps, {RATE} Hz, worst magnitude error {error:.4f} dB")
if error>.15: raise SystemExit("FIR validation failed")
