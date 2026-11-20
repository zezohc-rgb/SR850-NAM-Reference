Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
Name "SR850 System Reference - By Zezo HxCx"
OutFile "SR850-System-Reference-Setup.exe"
InstallDir "$PROGRAMFILES64\\SR850 System Reference"
RequestExecutionLevel admin
ShowInstDetails show
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "PortugueseBR"
Section "Instalar"
 SetOutPath "$INSTDIR"
 File "dist\\SR850-System-Reference.exe"
 File "vendor\\EqualizerAPO-x64-1.4.2.exe"
 ReadRegStr $0 HKLM "SOFTWARE\\EqualizerAPO" "InstallPath"
 ${If} $0 == ""
  DetailPrint "Instalando Equalizer APO oficial. O seletor de dispositivo pode ser exibido."
  ExecWait '"$INSTDIR\\EqualizerAPO-x64-1.4.2.exe" /S' $1
  ${If} $1 != 0
   MessageBox MB_ICONEXCLAMATION "O Equalizer APO retornou codigo $1. Verifique a instalacao."
  ${EndIf}
 ${Else}
  DetailPrint "Equalizer APO ja instalado em $0."
 ${EndIf}
 Delete "$INSTDIR\\EqualizerAPO-x64-1.4.2.exe"
 CreateDirectory "$SMPROGRAMS\\SR850 System Reference"
 CreateShortcut "$SMPROGRAMS\\SR850 System Reference\\SR850 System Reference.lnk" "$INSTDIR\\SR850-System-Reference.exe"
 CreateShortcut "$DESKTOP\\SR850 System Reference.lnk" "$INSTDIR\\SR850-System-Reference.exe"
 WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SR850SystemReference" "DisplayName" "SR850 System Reference - By Zezo HxCx"
 WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SR850SystemReference" "UninstallString" '"$INSTDIR\\Uninstall.exe"'
 WriteUninstaller "$INSTDIR\\Uninstall.exe"
 MessageBox MB_ICONINFORMATION "Instalacao concluida. No seletor de dispositivos do Equalizer APO, selecione apenas a saida M-Audio Fast Track MKII. Pode ser necessario reiniciar o Windows. A correcao permanece desligada ate voce ativa-la no SR850."
SectionEnd
Section "Uninstall"
 Delete "$DESKTOP\\SR850 System Reference.lnk"
 Delete "$SMPROGRAMS\\SR850 System Reference\\SR850 System Reference.lnk"
 RMDir "$SMPROGRAMS\\SR850 System Reference"
 Delete "$INSTDIR\\SR850-System-Reference.exe"
 Delete "$INSTDIR\\Uninstall.exe"
 RMDir "$INSTDIR"
 DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SR850SystemReference"
 ; Do not remove Equalizer APO, which may be used by other programs.
SectionEnd
