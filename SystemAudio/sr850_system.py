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
FIR_NAME = "SR850-Reference-48000.wav"

def bundled(name):
    return Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent)) / name

def find_apo():
    for root in [os.environ.get("ProgramFiles",r"C:\Program Files"),
                 os.environ.get("ProgramFiles(x86)",r"C:\Program Files (x86)")]:
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

def install(config_dir):
    config=config_dir/"config.txt"
    text,bom=read_config(config)
    text=remove_block(text)
    fir=bundled(FIR_NAME)
    if not fir.exists():
        raise RuntimeError("Filtro FIR não encontrado no pacote.")
    # Convolution takes a WAV impulse; same SR850 correction as the VST3.
    # Use relative paths inside Equalizer APO config directory.
    import shutil
    shutil.copyfile(fir,config_dir/FIR_NAME)
    (config_dir/CONFIG_NAME).write_text(
        "# SR850 Reference - FIR; correction only while app is running\n"
        "Preamp: -7 dB\n"
        f"Convolution: {FIR_NAME}\n",encoding="utf-8")
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
    def __init__(self,root):
        self.root=root
        self.active=False
        self.config_dir=find_apo()
        root.title("SR850 System Reference  |  By Zezo HxCx")
        root.geometry("540x380")
        root.resizable(False,False)
        root.configure(bg="#111317")
        self.label=tk.Label(root,text="SAMSON  SR850",bg="#111317",fg="#ead1b1",
                            font=("Segoe UI",26,"bold"))
        self.label.pack(pady=(26,2))
        tk.Label(root,text="SYSTEM REFERENCE  •  WINDOWS AUDIO",
                 bg="#111317",fg="#b18b5d",font=("Segoe UI",10)).pack()
        self.status=tk.StringVar(value="Correção desligada — áudio original")
        tk.Label(root,textvariable=self.status,bg="#111317",fg="#9fd3bb",
                 font=("Segoe UI",12),wraplength=490).pack(pady=(25,16))
        self.button=tk.Button(root,text="ATIVAR CORREÇÃO SR850",command=self.toggle,
                              bg="#aa7849",fg="white",activebackground="#c18e5c",
                              relief="flat",font=("Segoe UI",12,"bold"),height=2,width=29)
        self.button.pack()
        self.device=tk.StringVar()
        tk.Label(root,textvariable=self.device,bg="#111317",fg="#a4a9af",
                 wraplength=480,font=("Segoe UI",10)).pack(pady=13)
        tk.Label(root,text="Fechar = restaurar áudio original  |  By Zezo HxCx",
                 bg="#111317",fg="#7f858b",font=("Segoe UI",9)).pack(side="bottom",pady=17)
        root.protocol("WM_DELETE_WINDOW",self.close)
        self.update_device()
        if self.config_dir:
            # Clear any stale block from a previous crash before user enables correction.
            try: cleanup(self.config_dir/"config.txt")
            except Exception as exc: self.status.set("Aviso na recuperação: "+str(exc))
    def update_device(self):
        self.device.set("Equalizer APO: "+(str(self.config_dir) if self.config_dir else "não encontrado"))
    def toggle(self):
        if self.active:
            self.disable()
            return
        if not self.config_dir:
            messagebox.showerror(APP,"Instale Equalizer APO e selecione a saída de reprodução M-Audio no Configurator.\nDepois reabra o aplicativo.")
            return
        try:
            install(self.config_dir)
            launch_watchdog(self.config_dir/"config.txt")
            self.active=True
            self.status.set("Correção FIR ativa — saída configurada no Equalizer APO")
            self.button.configure(text="DESATIVAR / ÁUDIO ORIGINAL",bg="#416d5d")
        except Exception as exc:
            try: cleanup(self.config_dir/"config.txt")
            except Exception: pass
            messagebox.showerror(APP,str(exc)+"\n\nTalvez seja necessário executar como administrador.")
    def disable(self):
        if self.config_dir:
            try: cleanup(self.config_dir/"config.txt")
            except Exception as exc:
                messagebox.showerror(APP,"Falha ao restaurar: "+str(exc))
                return
        self.active=False
        self.status.set("Correção desligada — áudio original")
        self.button.configure(text="ATIVAR CORREÇÃO SR850",bg="#aa7849")
    def close(self):
        self.disable()
        if not self.active: self.root.destroy()

if __name__=="__main__":
    if len(sys.argv)==4 and sys.argv[1]=="--watchdog":
        watchdog(Path(sys.argv[3]),int(sys.argv[2]))
    else:
        root=tk.Tk()
        UI(root)
        root.mainloop()
