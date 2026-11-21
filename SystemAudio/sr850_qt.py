"""SR850 premium Qt controller. DSP and safe APO cleanup remain in sr850_system."""
import math
import sys
import subprocess
from pathlib import Path
from scipy.io import wavfile

import numpy as np
from PySide6.QtCore import Qt, QRectF, QPointF
from PySide6.QtGui import QColor, QPainter, QPen, QFont, QPainterPath, QBrush
from PySide6.QtWidgets import QApplication, QWidget, QMessageBox
import sr850_system as engine

BG="#101419"; PANEL="#1c242c"; GOLD="#eda35c"; TEXT="#f3e8d8"; MUTED="#9ba9b4"

class Controller(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SR850 System Reference  |  By Zezo HxCx")
        self.setMinimumSize(1020, 630)
        self.resize(1100, 680)
        self.active=False
        self.rate=44100
        self.gain=-6.0
        self.config_dir=engine.find_apo()
        self.message="AUDIO ORIGINAL  |  CORRECAO DESLIGADA"
        self.dragging=False
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
        w=self.width(); h=self.height()
        p.fillRect(self.rect(),QColor(BG))
        p.fillRect(0,0,w,66,QColor("#0a0e13"))
        p.setPen(QPen(QColor("#6a4528"),2)); p.drawLine(0,66,w,66)
        self.text(p,30,44,"SR850  SYSTEM REFERENCE",24,TEXT,True)
        self.text(p,w-190,40,"BY ZEZO HXCX",12,GOLD,True)
        self.panel(p,24,88,344,h-166)
        self.text(p,48,133,"SAMSON",20,GOLD,True)
        self.text(p,47,208,"SR850",66,TEXT,True)
        self.text(p,49,241,"REFERENCE HEADPHONE CORRECTION",11,MUTED)
        self.headphones(p,196,395)
        self.text(p,49,h-125,"LINEAR PHASE FIR",13,GOLD,True)
        self.text(p,49,h-99,"WINDOWS SYSTEM AUDIO",11,MUTED)
        left=390; right=w-24; graph_bottom=365
        self.panel(p,left,88,right-left,graph_bottom-88)
        self.text(p,left+22,125,"FREQUENCY RESPONSE",17,TEXT,True)
        self.text(p,right-195,124,"FIR RESPONSE / dB",11,MUTED)
        self.graph(p,QRectF(left+55,160,right-left-80,172))
        cy=385; ch=h-465
        self.panel(p,left,cy,185,ch)
        self.text(p,left+20,cy+33,"CORRECTION",14,MUTED,True)
        center=QPointF(left+92,cy+109)
        p.setPen(QPen(QColor("#35d994" if self.active else GOLD),5))
        p.setBrush(QColor("#10161d"));p.drawEllipse(center,43,43)
        self.text(p,left+74,cy+122,"⏻",35,"#35d994" if self.active else GOLD,True)
        self.text(p,left+40,cy+181,"ON" if self.active else "OFF",16,"#35d994" if self.active else MUTED,True)
        sx=left+201
        self.panel(p,sx,cy,195,ch)
        self.text(p,sx+17,cy+33,"SAMPLE RATE",14,MUTED,True)
        for i,(rate,label) in enumerate(((44100,"44.1"),(48000,"48"))):
            rect=QRectF(sx+13+i*91,cy+70,84,58)
            p.setPen(QPen(QColor(GOLD if self.rate==rate else "#4b5964"),2))
            p.setBrush(QColor("#59371e" if self.rate==rate else "#10161d"))
            p.drawRoundedRect(rect,8,8)
            self.text(p,rect.left()+13,rect.top()+35,label+"k",15,TEXT,True)
        gx=sx+211
        self.panel(p,gx,cy,right-gx,ch)
        self.text(p,gx+19,cy+33,"OUTPUT GAIN / HEADROOM",14,MUTED,True)
        self.text(p,gx+19,cy+95,f"{self.gain:+.1f} dB",31,GOLD,True)
        start=gx+24; end=right-25; y=cy+139
        p.setPen(QPen(QColor("#48515a"),7,Qt.SolidLine,Qt.RoundCap));p.drawLine(start,y,end,y)
        knob=start+(self.gain+12)/12*(end-start)
        p.setPen(QPen(QColor(GOLD),7,Qt.SolidLine,Qt.RoundCap));p.drawLine(start,y,int(knob),y)
        p.setBrush(QColor("#fff0d6"));p.setPen(QPen(QColor(GOLD),3))
        p.drawEllipse(QPointF(knob,y),12,12)
        self.text(p,start,cy+178,"-12 dB",10,MUTED)
        self.text(p,end-30,cy+178,"0 dB",10,MUTED)
        self.text(p,29,h-61,self.message[:95],12,"#35d994" if self.active else MUTED)
        self.text(p,w-255,h-61,"CONFIGURAR APO  ⚙",12,GOLD,True)
        self.text(p,29,h-25,"Fechar restaura o audio original • O grafico representa o FIR calculado, nao uma medicao do dispositivo",10,MUTED)
        p.end()

    def panel(self,p,x,y,w,h):
        p.setPen(QPen(QColor("#38414a"),1))
        p.setBrush(QColor(PANEL))
        p.drawRoundedRect(QRectF(x,y,w,h),15,15)

    def text(self,p,x,y,s,size,color,bold=False):
        f=QFont("Segoe UI",size)
        f.setBold(bold)
        p.setFont(f);p.setPen(QColor(color));p.drawText(QPointF(x,y),s)

    def headphones(self,p,x,y):
        p.setPen(QPen(QColor(GOLD),21,Qt.SolidLine,Qt.RoundCap))
        p.setBrush(Qt.NoBrush)
        p.drawArc(QRectF(x-108,y-116,216,225),0,180*16)
        p.setPen(QPen(QColor("#3b4651"),12));p.drawArc(QRectF(x-108,y-116,216,225),0,180*16)
        for cx in (x-107,x+107):
            p.setBrush(QColor("#0c1117"));p.setPen(QPen(QColor(GOLD),4))
            p.drawRoundedRect(QRectF(cx-33,y-14,66,112),27,27)
            p.setBrush(QColor("#343f49"));p.setPen(QPen(QColor("#74808b"),2))
            p.drawRoundedRect(QRectF(cx-21,y+1,42,82),18,18)

    def graph(self,p,r):
        import numpy as np
        p.save();p.setClipRect(r.adjusted(-5,-5,5,5))
        for db in (-12,-6,0,6,12):
            yy=r.bottom()-(db+12)/24*r.height()
            p.setPen(QPen(QColor("#34404a"),1));p.drawLine(QPointF(r.left(),yy),QPointF(r.right(),yy))
            self.text(p,r.left()-40,yy+4,f"{db:+}",9,MUTED)
        for hz in (20,50,100,200,500,1000,2000,5000,10000,20000):
            xx=r.left()+math.log10(hz/20)/3*r.width()
            p.setPen(QPen(QColor("#29343e"),1));p.drawLine(QPointF(xx,r.top()),QPointF(xx,r.bottom()))
        zero=r.bottom()-r.height()/2
        p.setPen(QPen(QColor("#8c9aa5"),1,Qt.DashLine))
        p.drawLine(QPointF(r.left(),zero),QPointF(r.right(),zero))
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

    def gain_from_x(self,x):
        gx=390+201+211
        left=gx+24;right=self.width()-49
        self.gain=round((max(0,min(1,(x-left)/max(1,right-left)))*12-12)*2)/2
        self.apply_live();self.update()

    def mousePressEvent(self,event):
        x=event.position().x();y=event.position().y()
        cy=385
        if 390+35<x<390+155 and cy+55<y<cy+165:
            self.toggle()
        elif 604<x<688 and cy+70<y<cy+128:
            self.change_rate(44100)
        elif 695<x<785 and cy+70<y<cy+128:
            self.change_rate(48000)
        elif x>self.width()-260 and y>self.height()-80:
            self.configure_apo()
        elif x>800 and cy+112<y<cy+165:
            self.dragging=True;self.gain_from_x(x)

    def mouseMoveEvent(self,event):
        if self.dragging:self.gain_from_x(event.position().x())

    def mouseReleaseEvent(self,event):
        self.dragging=False

    def wheelEvent(self,event):
        self.gain=max(-12,min(0,round((self.gain+(0.5 if event.angleDelta().y()>0 else -0.5))*2)/2))
        self.apply_live();self.update()

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
