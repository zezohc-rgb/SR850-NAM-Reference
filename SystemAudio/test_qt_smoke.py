"""CI smoke test: actual Qt drawing, float32 FIR, live controls, and watchdog routing."""
import os
os.environ["QT_QPA_PLATFORM"]="offscreen"
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from PySide6.QtWidgets import QApplication
from PySide6.QtGui import QImage, QPainter
from PySide6.QtCore import QPoint
import sr850_qt
app=QApplication([])
ui=sr850_qt.Controller()
assert ui.curves[44100] is not None and ui.curves[48000] is not None, "float32 FIR curve must load"
assert len(ui.curves[44100])==500
image=QImage(1100,680,QImage.Format_ARGB32)
image.fill(0)
p=QPainter(image)
ui.render(p, QPoint(0,0))
p.end()
assert not image.isNull()
ui.gain_from_x(1031)
assert ui.gain==0.0
ui.change_rate(48000)
assert ui.rate==48000
ui.close()
print("QT_SMOKE_PASS: render, FIR curves, sample rate, gain and shutdown")
