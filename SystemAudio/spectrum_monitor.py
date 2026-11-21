"""Read-only Windows WASAPI loopback spectrum. Never changes the output device."""
import threading
import time
import numpy as np

class SpectrumMonitor:
    def __init__(self):
        self.lock=threading.Lock()
        self.values=np.full(500,-90.0)
        self.status="INICIANDO"
        self.running=False
        self.thread=None
        self.freq=np.geomspace(20,20000,500)

    def start(self):
        if self.running:return
        self.running=True
        self.thread=threading.Thread(target=self._run,daemon=True,name="SR850-WASAPI")
        self.thread.start()

    def stop(self):
        self.running=False
        if self.thread:
            self.thread.join(timeout=1.5)

    def snapshot(self):
        with self.lock:
            return self.values.copy(),self.status

    def _run(self):
        try:
            import soundcard as sc
            while self.running:
                try:
                    speaker=sc.default_speaker()
                    if speaker is None:raise RuntimeError("Nenhuma saida padrao")
                    mic=sc.get_microphone(id=str(speaker.id),include_loopback=True)
                    if mic is None:raise RuntimeError("Loopback indisponivel")
                    with mic.recorder(samplerate=48000,blocksize=2048) as recorder:
                        self.status="WASAPI: "+speaker.name[:34]
                        while self.running:
                            samples=recorder.record(numframes=2048)
                            if samples is None or len(samples)==0:continue
                            mono=np.asarray(samples,dtype=np.float64)
                            if mono.ndim>1:mono=np.mean(mono,axis=1)
                            if len(mono)<64:continue
                            n=8192
                            window=np.hanning(len(mono))
                            fft=np.abs(np.fft.rfft(mono*window,n=n))
                            bins=np.fft.rfftfreq(n,1/48000)
                            # Display-only spectrum, referenced to full-scale sine.
                            amp=fft/max(1.0,np.sum(window)/2)
                            db=20*np.log10(np.maximum(amp,1e-8))
                            new=np.interp(self.freq,bins,db)
                            with self.lock:
                                rising=new>self.values
                                self.values=np.where(rising,.62*new+.38*self.values,.16*new+.84*self.values)
                except Exception as exc:
                    with self.lock:
                        self.status="WASAPI INDISPONIVEL: "+str(exc)[:48]
                        self.values=np.full(500,-90.0)
                    for _ in range(20):
                        if not self.running:break
                        time.sleep(.1)
        except Exception as exc:
            with self.lock:self.status="CAPTURA INDISPONIVEL: "+str(exc)[:50]
