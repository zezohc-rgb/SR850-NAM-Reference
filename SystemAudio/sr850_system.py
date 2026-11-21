"""SR850 System Reference — Windows system audio correction controller.
Requires Equalizer APO installed on the M-Audio playback endpoint.
Closing this app removes ONLY its own Include block; it never rewrites other EQ.
"""
import atexit
import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tkinter as tk
from tkinter import messagebox, filedialog

APP = "SR850 System Reference"
START = "# >>> SR850 SYSTEM REFERENCE MANAGED BEGIN >>>"
END = "# <<< SR850 SYSTEM REFERENCE MANAGED END <<<"
LINE = "Include: SR850-System-Reference.txt"
CONFIG_NAME = "SR850-System-Reference.txt"
FIR_NAMES = {44100: "SR850-Reference-44100.wav", 48000: "SR850-Reference-48000.wav"}

def bundled(name):
    return Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent)) / name

def find_apo():
    for root in dict.fromkeys([os.environ.get("ProgramW6432",r"C:\Program Files"),
                 os.environ.get("ProgramFiles",r"C:\Program Files"),
                 os.environ.get("ProgramFiles(x86)",r"C:\Program Files (x86)")]):
        path = Path(root) / "EqualizerAPO" / "config"
        if (path / "config.txt").is_file():
            return path
    return None

def remove_block(text):
    start = text.find(START)
    if start < 0:
        return text
    end = text.find(END, start)
    if end < 0:
        raise RuntimeError("Bloco gerenciado incompleto. Nenhum arquivo foi alterado.")
    return text[:start] + text[end+len(END):].lstrip("\r\n")

def atomic_write(path, data):
    temp = path.with_name(path.name + ".sr850-tmp")
    temp.write_bytes(data)
    os.replace(temp, path)

def read_config(path):
    data=path.read_bytes()
    # Preserve BOM and newline style. APO config is commonly UTF-8.
    bom=b"\xef\xbb\xbf" if data.startswith(b"\xef\xbb\xbf") else b""
    return data[len(bom):].decode("utf-8-sig"),bom

def write_config(path,text,bom):
    atomic_write(path,bom+text.encode("utf-8"))

def cleanup(config):
    text,bom=read_config(config)
    new=remove_block(text)
    if new != text:
        write_config(config,new,bom)

def install(config_dir, sample_rate=44100, preamp_db=-6.0):
    config=config_dir/"config.txt"
    text,bom=read_config(config)
    text=remove_block(text)
    fir_name=FIR_NAMES[sample_rate]
    fir=bundled(fir_name)
    if not fir.exists():
        raise RuntimeError("Filtro FIR não encontrado no pacote.")
    # Convolution takes a WAV impulse; same SR850 correction as the VST3.
    # Use relative paths inside Equalizer APO config directory.
    import shutil
    # FIR is immutable; avoid overwriting a WAV in use on every gain change.
    target_fir=config_dir/fir_name
    if not target_fir.is_file() or target_fir.stat().st_size != fir.stat().st_size:
        shutil.copyfile(fir,target_fir)
    # Replace the managed file atomically instead of truncating it in place.
    settings=(
        "# SR850 Reference - FIR; correction only while app is running\n"
        f"Preamp: {preamp_db:+.1f} dB\n"
        f"Convolution: {fir_name}\n"
    )
    atomic_write(config_dir/CONFIG_NAME,settings.encode("utf-8"))
    newline="\r\n" if "\r\n" in text else "\n"
    block=START+newline+LINE+newline+END+newline
    write_config(config,text.rstrip("\r\n")+newline+block,bom)

def watchdog(config, parent):
    # A detached child exits when its parent dies, including a forced termination.
    # Cleanup only the tagged Include block, leaving user EQ untouched.
    import time
    kernel=ctypes.windll.kernel32
    SYNCHRONIZE=0x00100000
    handle=kernel.OpenProcess(SYNCHRONIZE,False,parent)
    if handle:
        kernel.WaitForSingleObject(handle,0xFFFFFFFF)
        kernel.CloseHandle(handle)
    else:
        time.sleep(2)
    try: cleanup(config)
    except Exception: pass

def launch_watchdog(config):
    if getattr(sys,"frozen",False):
        cmd=[sys.executable,"--watchdog",str(os.getpid()),str(config)]
    else:
        cmd=[sys.executable,str(Path(__file__).resolve()),"--watchdog",str(os.getpid()),str(config)]
    flags=subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.DETACHED_PROCESS
    subprocess.Popen(cmd,creationflags=flags,stdin=subprocess.DEVNULL,
                     stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,close_fds=True)

class UI:
    BG = "#10151b"
    PANEL = "#1a222b"
    GOLD = "#ff9e34"
    TEXT = "#f2eee7"
    MUTED = "#a6b0bb"

    def __init__(self, root):
        self.root = root
        self.active = False
        self.config_dir = find_apo()
        self.rate = tk.StringVar(value="44.1 kHz")
        self.gain = tk.DoubleVar(value=-6.0)
        self.status = tk.StringVar(value="DESLIGADO  |  Audio original")
        root.title("SR850 System Reference | By Zezo HxCx")
        root.geometry("1040x630")
        root.minsize(940, 600)
        root.configure(bg=self.BG)
        self.canvas = tk.Canvas(root, bg=self.BG, highlightthickness=0, width=1040, height=630)
        self.canvas.pack(fill="both", expand=True)
        self.canvas.bind("<Configure>", lambda e: self.draw())
        self.canvas.bind("<Button-1>", self.click)
        self.canvas.bind("<B1-Motion>", self.drag_gain)
        self.canvas.bind("<MouseWheel>", self.wheel_gain)
        self.canvas.bind("<Button-4>", lambda e: self.adjust_gain(0.5))
        self.canvas.bind("<Button-5>", lambda e: self.adjust_gain(-0.5))
        root.protocol("WM_DELETE_WINDOW", self.close)
        if self.config_dir:
            try:
                cleanup(self.config_dir / "config.txt")
            except Exception as exc:
                self.status.set("Recuperacao pendente: " + str(exc))
        self.draw()

    def rect(self, x1, y1, x2, y2, fill, outline="", radius=0):
        c = self.canvas
        if radius:
            c.create_rectangle(x1+radius,y1,x2-radius,y2,fill=fill,outline="")
            c.create_rectangle(x1,y1+radius,x2,y2-radius,fill=fill,outline="")
            for x in (x1+radius,x2-radius):
                for y in (y1+radius,y2-radius):
                    c.create_oval(x-radius,y-radius,x+radius,y+radius,fill=fill,outline="")
        else:
            c.create_rectangle(x1,y1,x2,y2,fill=fill,outline=outline)

    def label(self,x,y,text,size=12,color=None,weight="normal",anchor="nw"):
        self.canvas.create_text(x,y,text=text,fill=color or self.TEXT,
                                font=("Segoe UI",size,weight),anchor=anchor)

    def draw(self):
        c=self.canvas
        c.delete("all")
        w=max(c.winfo_width(),940)
        h=max(c.winfo_height(),600)
        c.create_rectangle(0,0,w,h,fill=self.BG,outline="")
        c.create_rectangle(0,0,w,62,fill="#0a0f14",outline="")
        c.create_line(0,62,w,62,fill="#5b3921",width=2)
        self.label(30,15,"SR850  SYSTEM REFERENCE",22,weight="bold")
        self.label(w-26,24,"BY ZEZO HXCX",10,self.GOLD,anchor="ne")
        self.rect(24,85,365,h-80,self.PANEL,radius=15)
        self.label(52,112,"SAMSON",17,self.GOLD,"bold")
        self.label(52,145,"SR850",57,self.TEXT,"bold")
        self.label(53,223,"REFERENCE CORRECTION",12,self.MUTED)
        # Stylized headphone illustration drawn as vectors, no external assets.
        cx=194; cy=365
        c.create_arc(cx-104,cy-105,cx+104,cy+112,start=0,extent=180,
                     style="arc",outline="#e2a05c",width=19)
        c.create_arc(cx-104,cy-105,cx+104,cy+112,start=0,extent=180,
                     style="arc",outline="#3c4753",width=11)
        for x in (cx-101,cx+101):
            c.create_oval(x-35,cy-18,x+35,cy+98,fill="#0a0c0f",outline="#e09a4a",width=4)
            c.create_oval(x-23,cy,x+23,cy+78,fill="#28313a",outline="#65717b",width=2)
        self.label(54,h-142,"FIR  /  LINEAR PHASE",11,self.GOLD,"bold")
        self.label(54,h-119,"Windows System Audio",11,self.MUTED)
        x0=390
        graph_right=w-25
        self.rect(x0,85,graph_right,365,self.PANEL,radius=15)
        self.label(x0+24,102,"RESPOSTA DO FILTRO FIR",15,weight="bold")
        self.label(graph_right-20,108,"Curva calculada do WAV",10,self.MUTED,anchor="ne")
        self.draw_graph(x0+53,155,graph_right-25,335)
        y=388
        self.rect(x0,y,x0+190,h-80,self.PANEL,radius=14)
        self.label(x0+22,y+18,"CORRECAO",13,self.MUTED,"bold")
        color="#35ce8a" if self.active else self.GOLD
        c.create_oval(x0+51,y+52,x0+139,y+140,fill="#0b1016",outline=color,width=6)
        self.label(x0+95,y+74,"⏻",34,color,"bold","n")
        self.label(x0+95,y+159,"ATIVADA" if self.active else "DESLIGADA",
                   13,color,"bold","n")
        sx=x0+207
        self.rect(sx,y,sx+205,h-80,self.PANEL,radius=14)
        self.label(sx+18,y+18,"AMOSTRAGEM",13,self.MUTED,"bold")
        for i,rate in enumerate(("44.1 kHz","48 kHz")):
            bx=sx+16+i*94
            selected=self.rate.get()==rate
            c.create_rectangle(bx,y+76,bx+89,y+124,
                               fill="#58351b" if selected else "#10151b",
                               outline=self.GOLD if selected else "#65717b",width=2)
            self.label(bx+44,y+89,rate,12,self.TEXT,"bold","n")
        gx=sx+221
        self.rect(gx,y,graph_right,h-80,self.PANEL,radius=14)
        self.label(gx+18,y+18,"GANHO / HEADROOM",13,self.MUTED,"bold")
        self.label(gx+25,y+66,f"{self.gain.get():+.1f} dB",26,self.GOLD,"bold")
        bar_left=gx+22; bar_right=graph_right-22; bar_y=y+134
        c.create_line(bar_left,bar_y,bar_right,bar_y,fill="#48535f",width=7)
        knob_x=bar_left+(self.gain.get()+12)/12*(bar_right-bar_left)
        c.create_line(bar_left,bar_y,knob_x,bar_y,fill=self.GOLD,width=7)
        c.create_oval(knob_x-11,bar_y-11,knob_x+11,bar_y+11,
                     fill="#f9e5c9",outline=self.GOLD,width=3)
        self.label(bar_left,y+153,"-12 dB",10,self.MUTED)
        self.label(bar_right,y+153,"0 dB",10,self.MUTED,anchor="ne")
        self.label(30,h-53,self.status.get(),11,
                   "#35ce8a" if self.active else self.MUTED)
        apo="APO localizado" if self.config_dir else "APO nao encontrado"
        self.label(w-28,h-53,apo+"  |  CONFIGURAR",11,
                   "#35ce8a" if self.config_dir else "#f4a45b",anchor="ne")
        self.label(w/2,h-22,"Fechar restaura o audio original  •  A curva mostra somente o FIR, sem medir o fone",
                   10,self.MUTED,anchor="center")

    def draw_graph(self,x1,y1,x2,y2):
        import math
        c=self.canvas
        for db in (-12,-6,0,6,12):
            yy=y2-(db+12)/24*(y2-y1)
            c.create_line(x1,yy,x2,yy,fill="#34404a")
            self.label(x1-7,yy,f"{db:+} dB",9,self.MUTED,anchor="e")
        for hz in (20,50,100,200,500,1000,2000,5000,10000,20000):
            xx=x1+math.log(hz/20)/math.log(1000)*(x2-x1)
            c.create_line(xx,y1,xx,y2,fill="#29333d")
            self.label(xx,y2+7,f"{hz//1000}k" if hz>=1000 else str(hz),
                       9,self.MUTED,anchor="n")
        try:
            import wave
            import numpy as np
            with wave.open(str(bundled(FIR_NAMES[44100 if self.rate.get()=="44.1 kHz" else 48000])),"rb") as wav:
                raw=wav.readframes(wav.getnframes())
                width=wav.getsampwidth()
                channels=wav.getnchannels()
                fs=wav.getframerate()
            if width==2:
                samples=np.frombuffer(raw,dtype="<i2").astype(float)/32768
            elif width==4:
                samples=np.frombuffer(raw,dtype="<i4").astype(float)/2147483648
            else:
                raise ValueError("Unsupported WAV sample width")
            if channels>1:
                samples=samples.reshape(-1,channels).mean(axis=1)
            n=max(65536,1 << (len(samples)-1).bit_length())
            spec=np.fft.rfft(samples,n=n)
            freq=np.fft.rfftfreq(n,1/fs)
            pts=[]
            for i in range(260):
                hz=20*1000**(i/259)
                mag=float(np.interp(hz,freq,np.abs(spec)))
                db=20*math.log10(max(mag,1e-9))+self.gain.get()
                xx=x1+i/259*(x2-x1)
                yy=y2-(max(-12,min(12,db))+12)/24*(y2-y1)
                pts.extend((xx,yy))
            c.create_line(*pts,fill=self.GOLD,width=3,smooth=True)
        except Exception:
            self.label((x1+x2)/2,(y1+y2)/2,
                       "FIR indisponivel para visualizacao",11,self.MUTED,anchor="center")

    def adjust_gain(self,step):
        value=round(max(-12,min(0,self.gain.get()+step))*2)/2
        if value != self.gain.get():
            self.gain.set(value)
            self.apply_live()
            self.draw()

    def wheel_gain(self,event):
        self.adjust_gain(0.5 if event.delta>0 else -0.5)

    def drag_gain(self,event):
        w=max(self.canvas.winfo_width(),940)
        gx=390+207+221
        y=388
        if gx+15<=event.x<=w-25 and y+110<=event.y<=y+160:
            left=gx+22; right=w-47
            value=-12+12*max(0,min(1,(event.x-left)/(right-left)))
            self.gain.set(round(value*2)/2)
            self.apply_live()
            self.draw()

    def click(self,event):
        x,y=event.x,event.y
        w=max(self.canvas.winfo_width(),940)
        h=max(self.canvas.winfo_height(),600)
        if 390+40<=x<=390+150 and 440<=y<=550:
            self.toggle()
        elif 597+16<=x<=597+105 and 464<=y<=512:
            self.change_rate("44.1 kHz")
        elif 597+110<=x<=597+200 and 464<=y<=512:
            self.change_rate("48 kHz")
        elif x>=w-255 and y>=h-70:
            self.configure_apo()
        else:
            self.drag_gain(event)

    def change_rate(self,rate):
        self.rate.set(rate)
        self.apply_live()
        self.draw()

    def apply_live(self):
        if not self.active:
            return
        try:
            rate=44100 if self.rate.get()=="44.1 kHz" else 48000
            install(self.config_dir,rate,float(self.gain.get()))
            self.status.set(f"FIR {rate} Hz configurado no APO (saida nao verificada)")
        except Exception as exc:
            self.status.set("Erro ao atualizar: "+str(exc))
            messagebox.showerror(APP,str(exc))

    def configure_apo(self):
        for base in (os.environ.get("ProgramW6432",r"C:\Program Files"),
                     os.environ.get("ProgramFiles",r"C:\Program Files"),
                     os.environ.get("ProgramFiles(x86)",r"C:\Program Files (x86)")):
            exe=Path(base)/"EqualizerAPO"/"Configurator.exe"
            if exe.is_file():
                try:
                    subprocess.Popen([str(exe)])
                except Exception as exc:
                    messagebox.showerror(APP,str(exc))
                return
        messagebox.showwarning(APP,"Equalizer APO Configurator nao encontrado.")

    def toggle(self):
        if self.active:
            self.disable()
            return
        if not self.config_dir:
            messagebox.showerror(APP,"Instale o Equalizer APO e configure a saida de reproducao desejada.")
            return
        try:
            rate=44100 if self.rate.get()=="44.1 kHz" else 48000
            install(self.config_dir,rate,float(self.gain.get()))
            launch_watchdog(self.config_dir/"config.txt")
            self.active=True
            self.status.set(f"FIR {rate} Hz configurado no APO (saida nao verificada)")
        except Exception as exc:
            try: cleanup(self.config_dir/"config.txt")
            except Exception: pass
            messagebox.showerror(APP,str(exc))
        self.draw()

    def disable(self):
        if self.config_dir:
            try:
                cleanup(self.config_dir/"config.txt")
            except Exception as exc:
                messagebox.showerror(APP,"Falha ao restaurar: "+str(exc))
                return
        self.active=False
        self.status.set("DESLIGADO  |  Audio original")
        self.draw()

    def close(self):
        self.disable()
        if not self.active:
            self.root.destroy()

if __name__=="__main__":
    if len(sys.argv)==4 and sys.argv[1]=="--watchdog":
        watchdog(Path(sys.argv[3]),int(sys.argv[2]))
    else:
        root=tk.Tk()
        UI(root)
        root.mainloop()
