Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
Name "SR850 System Reference - By Zezo HxCx"
OutFile "SR850-System-Reference-Setup.exe"
InstallDir "$PROGRAMFILES64\SR850 System Reference"
RequestExecutionLevel admin
ShowInstDetails show
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "PortugueseBR"
Section "Instalar"
 SetOutPath "$INSTDIR"
 File "dist\SR850-System-Reference.exe"

 ; Install dependency on the user's machine instead of downloading from SourceForge on CI.
 ; Winget handles source selection and installer verification.
 IfFileExists "$PROGRAMFILES64\EqualizerAPO\config\config.txt" apo_installed
 IfFileExists "$PROGRAMFILES\EqualizerAPO\config\config.txt" apo_installed
 DetailPrint "Instalando Equalizer APO via Windows Package Manager..."
 nsExec::ExecToLog 'winget install --id EqualizerAPO.EqualizerAPO --exact --source winget --accept-package-agreements --accept-source-agreements --silent --disable-interactivity'
 Pop $1
 ${If} $1 != 0
   MessageBox MB_ICONSTOP "Nao foi possivel instalar o Equalizer APO automaticamente (codigo $1). Verifique a conexao e o Windows Package Manager. O SR850 nao sera ativado."
   Abort
 ${EndIf}
 apo_installed:
 CreateDirectory "$SMPROGRAMS\SR850 System Reference"
 CreateShortcut "$SMPROGRAMS\SR850 System Reference\SR850 System Reference.lnk" "$INSTDIR\SR850-System-Reference.exe"
 CreateShortcut "$DESKTOP\SR850 System Reference.lnk" "$INSTDIR\SR850-System-Reference.exe"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SR850SystemReference" "DisplayName" "SR850 System Reference - By Zezo HxCx"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SR850SystemReference" "UninstallString" '"$INSTDIR\Uninstall.exe"'
 WriteUninstaller "$INSTDIR\Uninstall.exe"
 MessageBox MB_ICONINFORMATION "Instalacao concluida. No seletor de dispositivos do Equalizer APO, selecione apenas a saida M-Audio Fast Track MKII. Pode ser necessario reiniciar o Windows. A correcao permanece desligada ate voce ativa-la no SR850."
SectionEnd
Section "Uninstall"
 Delete "$DESKTOP\SR850 System Reference.lnk"
 Delete "$SMPROGRAMS\SR850 System Reference\SR850 System Reference.lnk"
 RMDir "$SMPROGRAMS\SR850 System Reference"
 Delete "$INSTDIR\SR850-System-Reference.exe"
 Delete "$INSTDIR\Uninstall.exe"
 RMDir "$INSTDIR"
 DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SR850SystemReference"
 ; Do not remove Equalizer APO, which may be used by other programs.
SectionEnd
