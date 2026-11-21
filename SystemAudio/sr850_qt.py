"""SR850 premium Qt controller. DSP and safe APO cleanup remain in sr850_system."""
import math
import os
import sys
import subprocess
from pathlib import Path
from scipy.io import wavfile

import numpy as np
from PySide6.QtCore import Qt, QRectF, QPointF, QTimer
from PySide6.QtGui import QColor, QPainter, QPen, QFont, QPainterPath, QBrush, QLinearGradient, QRadialGradient
from PySide6.QtWidgets import QApplication, QWidget, QMessageBox
import sr850_system as engine
from spectrum_monitor import SpectrumMonitor

BG="#101419"; PANEL="#1c242c"; GOLD="#eda35c"; TEXT="#f3e8d8"; MUTED="#9ba9b4"

class Controller(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SR850 System Reference  |  By Zezo HxCx")
        self.setMinimumSize(990, 612)
        self.resize(1100, 680)
        self.active=False
        self.rate=44100
        self.gain=0.0
        self.config_dir=engine.find_apo()
        self.message="AUDIO ORIGINAL  |  CORRECAO DESLIGADA"
        self.dragging=False
        self.monitor=SpectrumMonitor()
        self.spectrum=np.full(500,-90.0)
        self.monitor_status='WASAPI: aguardando'
        self.frame_timer=QTimer(self)
        self.frame_timer.timeout.connect(self.refresh_spectrum)
        if os.environ.get('QT_QPA_PLATFORM')!='offscreen':
            self.monitor.start()
            self.frame_timer.start(45)
        self.curves={}
        for rate in (44100,48000):
            self.curves[rate]=self.load_curve(rate)
        if self.config_dir:
            try:
                engine.cleanup(self.config_dir/"config.txt")
            except Exception as exc:
                self.message="RECUPERACAO: "+str(exc)

    def load_curve(self,rate):
        try:
            fs, values = wavfile.read(str(engine.bundled(engine.FIR_NAMES[rate])))
            if values.dtype.kind == "f":
                values = values.astype(np.float64)
            elif values.dtype.kind in "iu":
                values = values.astype(np.float64) / float(2 ** (8 * values.dtype.itemsize - 1))
            else:
                raise ValueError("Unsupported FIR WAV encoding")
            if values.ndim > 1:
                values = values.mean(axis=1)
            n=max(65536,1<<(len(values)-1).bit_length())
            magnitude=np.abs(np.fft.rfft(values,n=n))
            bins=np.fft.rfftfreq(n,1/fs)
            frequencies=np.geomspace(20,20000,500)
            db=20*np.log10(np.maximum(np.interp(frequencies,bins,magnitude),1e-10))
            return db
        except Exception:
            return None

    def paintEvent(self,event):
        p=QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        # Design space is fixed and uniformly scaled, keeping hit targets aligned.
        p.fillRect(self.rect(),QColor("#070a0d"))
        scale=min(self.width()/1100,self.height()/680)
        ox=(self.width()-1100*scale)/2
        oy=(self.height()-680*scale)/2
        p.translate(ox,oy);p.scale(scale,scale)
        p.setPen(Qt.NoPen)
        shell=QLinearGradient(0,0,1100,680)
        shell.setColorAt(0,QColor("#252c32"));shell.setColorAt(.52,QColor("#0c1117"))
        shell.setColorAt(1,QColor("#232b32"))
        p.setBrush(shell);p.drawRoundedRect(QRectF(8,8,1084,664),22,22)
        p.setPen(QPen(QColor("#6e7477"),2));p.setBrush(Qt.NoBrush)
        p.drawRoundedRect(QRectF(9,9,1082,662),22,22)
        p.setPen(Qt.NoPen)
        p.setBrush(QColor("#0b1015"));p.drawRoundedRect(QRectF(23,23,1054,634),16,16)
        # Metallic header and narrow amber illuminated rule
        header=QLinearGradient(0,30,0,102)
        header.setColorAt(0,QColor("#323c44"));header.setColorAt(1,QColor("#111820"))
        p.setBrush(header);p.drawRoundedRect(QRectF(28,28,1044,72),12,12)
        p.setPen(QPen(QColor("#b46d35"),2));p.drawLine(44,102,1055,102)
        self.text(p,54,69,"SR850",31,"#f4f1ea",True)
        self.text(p,180,67,"SYSTEM REFERENCE",18,"#d9a36c",True)
        self.text(p,181,87,"PRECISION HEADPHONE CALIBRATION",9,"#99a4ac")
        self.text(p,903,64,"ZEZO HXCX",13,"#e9b77e",True)
        self.text(p,917,83,"STUDIO EDITION",9,"#98a5ad")
        # Left feature bay: photo-like sculpted headphone illustration.
        self.panel(p,35,119,363,449)
        self.text(p,58,157,"SAMSON  /  SR850",17,"#f0e7da",True)
        self.text(p,58,181,"OPEN BACK REFERENCE",10,"#98a4ab")
        self.headphones(p,215,354)
        self.text(p,62,529,"CALIBRATED LISTENING",13,"#e8a366",True)
        self.text(p,62,547,"Minimum-phase FIR  •  System wide",10,"#9ca9b1")
        # Graph occupies the hero space, rather than a generic control card.
        self.panel(p,412,119,651,302)
        self.text(p,440,154,"FREQUENCY RESPONSE",17,"#f2e8d8",True)
        self.text(p,870,152,"20 Hz — 20 kHz",11,"#d5a06a")
        self.text(p,441,175,"MEASURED FIR TRANSFER  /  LOG FREQUENCY",9,"#8b9aa5")
        self.graph(p,QRectF(473,206,554,163))
        self.text(p,473,405,"—  CORRECTION FIR",10,"#e8a366")
        self.text(p,666,405,"—  LIVE SPECTRUM (WASAPI)",10,"#5acde0")
        # Controls use a real rack-style layout with separate insets.
        self.panel(p,412,434,651,134)
        self.text(p,439,461,"CORRECTION",11,"#abb6be",True)
        cx=472;cy=511
        glow=QRadialGradient(cx,cy,43)
        glow.setColorAt(0,QColor("#3c7d61" if self.active else "#65452b"))
        glow.setColorAt(1,QColor("#0b1015"))
        p.setPen(QPen(QColor("#64dba3" if self.active else "#df9a54"),3))
        p.setBrush(glow);p.drawEllipse(QPointF(cx,cy),30,30)
        # Native power glyph: independent of installed font symbols.
        p.setPen(QPen(QColor("#d6ffea" if self.active else "#ffe2ba"),4,Qt.SolidLine,Qt.RoundCap))
        p.setBrush(Qt.NoBrush)
        p.drawArc(QRectF(cx-12,cy-10,24,25),135*16,270*16)
        p.drawLine(QPointF(cx,cy-16),QPointF(cx,cy+1))
        self.text(p,520,508,"ACTIVE" if self.active else "BYPASS",15,"#65e6a5" if self.active else "#e4a76e",True)
        self.text(p,520,528,"FIR ENGAGED" if self.active else "ORIGINAL AUDIO",9,"#8d9ca7")
        p.setPen(QPen(QColor("#46525c"),1));p.drawLine(653,454,653,551)
        self.text(p,674,461,"SAMPLE RATE",11,"#abb6be",True)
        for i,(rate,label) in enumerate(((44100,"44.1k"),(48000,"48k"))):
            r=QRectF(672+i*83,486,75,47)
            g=QLinearGradient(r.topLeft(),r.bottomLeft())
            selected=self.rate==rate
            g.setColorAt(0,QColor("#8b552f" if selected else "#323b44"))
            g.setColorAt(1,QColor("#38241a" if selected else "#141a20"))
            p.setPen(QPen(QColor("#e9aa6d" if selected else "#48535c"),2))
            p.setBrush(g);p.drawRoundedRect(r,7,7)
            self.text(p,r.left()+12,r.top()+29,label,13,"#ffe7c9" if selected else "#a8b3ba",True)
        p.setPen(QPen(QColor("#46525c"),1));p.drawLine(848,454,848,551)
        self.text(p,868,461,"OUTPUT GAIN",11,"#abb6be",True)
        self.text(p,868,494,f"{self.gain:+.1f} dB",23,"#eeb57a",True)
        x1=873;x2=1031; yy=527
        p.setPen(QPen(QColor("#46525c"),6,Qt.SolidLine,Qt.RoundCap));p.drawLine(x1,yy,x2,yy)
        knob=x1+(self.gain+12)/12*(x2-x1)
        p.setPen(QPen(QColor("#e3a15e"),6,Qt.SolidLine,Qt.RoundCap));p.drawLine(x1,yy,int(knob),yy)
        p.setBrush(QColor("#f5d1a2"));p.setPen(QPen(QColor("#6c4225"),3));p.drawEllipse(QPointF(knob,yy),10,10)
        self.text(p,870,552,"-12",9,"#99a5ae")
        self.text(p,1013,552,"0 dB",9,"#99a5ae")
        # Footer: persistent state, APO launcher, and safety hint.
        p.setPen(Qt.NoPen);p.setBrush(QColor("#111820"))
        p.drawRoundedRect(QRectF(35,581,1028,58),9,9)
        p.setBrush(QColor("#58d69b" if self.active else "#d49a63"))
        p.drawEllipse(QPointF(57,603),5,5)
        self.text(p,73,601,self.message[:72],10,"#c2d1d9")
        self.text(p,73,619,self.monitor_status[:80],9,"#68b5c8")
        p.setPen(QPen(QColor("#a36a3f"),1))
        p.setBrush(QColor("#31251f"));p.drawRoundedRect(QRectF(857,590,183,33),7,7)
        self.text(p,878,611,"CONFIGURAR APO",12,"#e8ad75",True)
        self.text(p,49,653,"ON/OFF controla o Equalizer APO  •  Fechar restaura o audio original",9,"#81919d")
        p.end()

    def panel(self,p,x,y,w,h):
        fill=QLinearGradient(x,y,x+w,y+h)
        fill.setColorAt(0,QColor("#222c34"))
        fill.setColorAt(.5,QColor("#121a22"))
        fill.setColorAt(1,QColor("#10161d"))
        p.setPen(QPen(QColor("#45505a"),1))
        p.setBrush(fill);p.drawRoundedRect(QRectF(x,y,w,h),13,13)

    def text(self,p,x,y,s,size,color,bold=False):
        f=QFont("Segoe UI",size)
        f.setBold(bold)
        p.setFont(f);p.setPen(QColor(color));p.drawText(QPointF(x,y),s)

    def headphones(self,p,x,y):
        # Layered metal/plastic rendering with grille perforations and lighting.
        halo=QRadialGradient(x,y,190)
        halo.setColorAt(0,QColor(121,78,47,85))
        halo.setColorAt(1,QColor(0,0,0,0))
        p.setPen(Qt.NoPen);p.setBrush(halo)
        p.drawEllipse(QPointF(x,y),183,183)
        for width,color,dy in ((32,"#0a0d10",0),(23,"#626b71",3),(14,"#1c242b",7),(5,"#d1a478",9)):
            p.setPen(QPen(QColor(color),width,Qt.SolidLine,Qt.RoundCap))
            p.setBrush(Qt.NoBrush)
            p.drawArc(QRectF(x-112,y-124+dy,224,240),0,180*16)
        for cx in (x-108,x+108):
            p.setPen(QPen(QColor("#c08b5d"),3))
            body=QLinearGradient(cx-40,y-24,cx+43,y+103)
            body.setColorAt(0,QColor("#5d6467"))
            body.setColorAt(.4,QColor("#11181e"))
            body.setColorAt(1,QColor("#454a4b"))
            p.setBrush(body)
            p.drawRoundedRect(QRectF(cx-41,y-25,82,132),29,29)
            p.setPen(QPen(QColor("#090e13"),8))
            p.setBrush(QColor("#1a2229"))
            p.drawRoundedRect(QRectF(cx-29,y-12,58,106),25,25)
            p.setPen(QPen(QColor("#4d5962"),1))
            for row in range(8):
                for col in range(3):
                    px=cx-14+col*14
                    py=y+2+row*10
                    p.setBrush(QColor("#69737a" if (row+col)%3==0 else "#303b43"))
                    p.drawEllipse(QPointF(px,py),2.2,2.2)
        p.setPen(QPen(QColor("#9a7554"),3))
        p.drawLine(QPointF(x-110,y+99),QPointF(x-107,y+147))

    def graph(self,p,r):
        import numpy as np
        p.save();p.setClipRect(r.adjusted(-5,-5,5,5))
        for db in (-12,-6,0,6,12):
            yy=r.bottom()-(db+12)/24*r.height()
            p.setPen(QPen(QColor("#34404a"),1));p.drawLine(QPointF(r.left(),yy),QPointF(r.right(),yy))
            self.text(p,r.left()-33,yy+4,f"{db:+}",9,MUTED)
        for hz in (20,50,100,200,500,1000,2000,5000,10000,20000):
            xx=r.left()+math.log10(hz/20)/3*r.width()
            p.setPen(QPen(QColor("#29343e"),1));p.drawLine(QPointF(xx,r.top()),QPointF(xx,r.bottom()))
        zero=r.bottom()-r.height()/2
        p.setPen(QPen(QColor("#8c9aa5"),1,Qt.DashLine))
        p.drawLine(QPointF(r.left(),zero),QPointF(r.right(),zero))
        # Live audio spectrum in dBFS, independently scaled from FIR gain.
        spectrum=self.spectrum
        if spectrum is not None and len(spectrum)>1:
            path=QPainterPath()
            for i,db in enumerate(spectrum):
                xx=r.left()+i/(len(spectrum)-1)*r.width()
                yy=r.bottom()-max(0,min(1,(float(db)+90)/90))*r.height()
                if i==0:path.moveTo(xx,yy)
                else:path.lineTo(xx,yy)
            fill=QPainterPath(path)
            fill.lineTo(r.right(),r.bottom())
            fill.lineTo(r.left(),r.bottom())
            fill.closeSubpath()
            glow=QLinearGradient(0,r.top(),0,r.bottom())
            glow.setColorAt(0,QColor(44,215,250,92))
            glow.setColorAt(1,QColor(18,128,180,3))
            p.setPen(Qt.NoPen);p.setBrush(glow);p.drawPath(fill)
            p.setBrush(Qt.NoBrush)
            p.setPen(QPen(QColor("#164f68"),4));p.drawPath(path)
            p.setPen(QPen(QColor("#58dafa"),1.6));p.drawPath(path)
        curve=self.curves[self.rate]
        if self.active and curve is not None:
            path=QPainterPath()
            for i,db in enumerate(curve):
                xx=r.left()+i/(len(curve)-1)*r.width()
                yy=r.bottom()-(max(-12,min(12,float(db)+self.gain))+12)/24*r.height()
                if i==0:path.moveTo(xx,yy)
                else:path.lineTo(xx,yy)
            p.setPen(QPen(QColor(GOLD),3))
            p.drawPath(path)
        elif self.active:
            self.text(p,r.left()+80,r.center().y(),"FIR unavailable",12,MUTED)
        p.restore()
        for hz in (20,100,1000,10000,20000):
            xx=r.left()+math.log10(hz/20)/3*r.width()
            self.text(p,xx-12,r.bottom()+20,f"{hz//1000}k" if hz>=1000 else str(hz),9,MUTED)

    def refresh_spectrum(self):
        self.spectrum,self.monitor_status=self.monitor.snapshot()
        self.update()

    def design_pos(self,event):
        scale=min(self.width()/1100,self.height()/680)
        ox=(self.width()-1100*scale)/2
        oy=(self.height()-680*scale)/2
        return ((event.position().x()-ox)/scale,(event.position().y()-oy)/scale)

    def gain_from_x(self,x):
        self.gain=round((max(0,min(1,(x-873)/158))*12-12)*2)/2
        self.apply_live()
        self.update()

    def mousePressEvent(self,event):
        x,y=self.design_pos(event)
        if 435<x<620 and 474<y<550:
            self.toggle()
        elif 672<x<747 and 486<y<533:
            self.change_rate(44100)
        elif 755<x<830 and 486<y<533:
            self.change_rate(48000)
        elif 857<x<1040 and 590<y<623:
            self.configure_apo()
        elif 860<x<1045 and 510<y<550:
            self.dragging=True
            self.gain_from_x(x)

    def mouseMoveEvent(self,event):
        if self.dragging:
            x,_=self.design_pos(event)
            self.gain_from_x(x)

    def mouseReleaseEvent(self,event):
        self.dragging=False

    def wheelEvent(self,event):
        x,y=self.design_pos(event)
        if 857<x<1050 and 450<y<570:
            self.gain=max(-12,min(0,round((self.gain+(0.5 if event.angleDelta().y()>0 else -0.5))*2)/2))
            self.apply_live()
            self.update()

    def change_rate(self,rate):
        self.rate=rate
        self.apply_live()
        self.update()

    def apply_live(self):
        if not self.active:return
        try:
            engine.install(self.config_dir,self.rate,self.gain)
            self.message=f"FIR {self.rate} Hz configurado no APO • Saida nao verificada"
        except Exception as exc:
            self.message="FALHA AO ATUALIZAR"
            QMessageBox.critical(self,"SR850",str(exc))

    def toggle(self):
        if self.active:
            self.disable()
        elif not self.config_dir:
            QMessageBox.warning(self,"SR850","Equalizer APO nao encontrado. Configure o dispositivo no Configurator.")
        else:
            try:
                engine.install(self.config_dir,self.rate,self.gain)
                engine.launch_watchdog(self.config_dir/"config.txt")
                self.active=True
                self.message=f"FIR {self.rate} Hz configurado no APO • Saida nao verificada"
            except Exception as exc:
                try:engine.cleanup(self.config_dir/"config.txt")
                except Exception:pass
                QMessageBox.critical(self,"SR850",str(exc))
        self.update()

    def disable(self):
        if self.config_dir:
            try:engine.cleanup(self.config_dir/"config.txt")
            except Exception as exc:
                QMessageBox.critical(self,"Restauracao falhou",str(exc))
                return False
        self.active=False
        self.message="AUDIO ORIGINAL  |  CORRECAO DESLIGADA"
        self.update()
        return True

    def configure_apo(self):
        for root in (r"C:\Program Files",r"C:\Program Files (x86)"):
            exe=Path(root)/"EqualizerAPO"/"Configurator.exe"
            if exe.is_file():
                subprocess.Popen([str(exe)])
                return
        QMessageBox.warning(self,"SR850","Configurator do Equalizer APO nao encontrado.")

    def closeEvent(self,event):
        self.frame_timer.stop()
        self.monitor.stop()
        if self.disable():event.accept()
        else:event.ignore()

if __name__=="__main__":
    if len(sys.argv)==4 and sys.argv[1]=="--watchdog":
        engine.watchdog(Path(sys.argv[3]), int(sys.argv[2]))
        sys.exit(0)
    app=QApplication(sys.argv)
    app.setStyle("Fusion")
    window=Controller()
    window.show()
    sys.exit(app.exec())
