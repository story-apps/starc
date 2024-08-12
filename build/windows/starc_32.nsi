;NSIS Modern User Interface
;Welcome/Finish Page Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  
  ; Скрипт для регистрации ассоциаций файлов
  ; Взят отсюда http://nsis.sourceforge.net/File_Association , положить сюда "Program Files\NSIS\Include"
  !include "FileAssociation.nsh"

;--------------------------------
;General

  ;Show all languages, despite user's codepage
  !define MUI_LANGDLL_ALLLANGUAGES

  !define pkgdir "files_32"

  ;Name and file
  Name "Story Architect"
  Caption "Installing application for creating stories"
  OutFile "starc-setup-32.exe"
  BrandingText "Story Apps LLC"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Starc"
  InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "UninstallString"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  Function createShortcutsFunction
  CreateDirectory "$SMPROGRAMS\Starc"
  CreateShortCut "$SMPROGRAMS\Starc\Story Architect.lnk" "$INSTDIR\starcapp.exe"
  CreateShortCut "$SMPROGRAMS\Starc\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$DESKTOP\Story Architect.lnk" "$INSTDIR\starcapp.exe" "" "$INSTDIR\starcapp.exe" 0
  FunctionEnd

  !define MUI_ABORTWARNING
  !define MUI_FINISHPAGE_RUN "$INSTDIR\starcapp.exe"
  !define MUI_FINISHPAGE_SHOWREADME ""
  !define MUI_FINISHPAGE_SHOWREADME_CHECKED
  !define MUI_FINISHPAGE_SHOWREADME_TEXT "Create Desktop and Start Menu Shortcuts"
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION createShortcutsFunction

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Additional includes


!include FileAssociation.nsh

;--------------------------------
;Installer Sections

Section "App files section" SecFiles

  ; Добавление программы в список установленных программ системы
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "DisplayName" "Story Architect"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "Publisher" "Story Apps"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "DisplayVersion" "$%APP_VERSION%"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "Version" "$%APP_VERSION%"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "DisplayIcon" "$INSTDIR\starcapp.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "UrlInfoAbout" "https://starc.app"
  
  SetOutPath "$INSTDIR"
  File /r "${pkgdir}\"

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Регистрируем ассоциации 
  ${registerExtension} "$INSTDIR\starcapp.exe" ".starc" "Story Architect project"
  
  ; Обновляем эксплорер
  System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'

SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; Удалим программу из списка установленных
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc"
  
  ; Удаляем все файлы кроме базы данных
  Delete "$INSTDIR\audio\*.*"
  Delete "$INSTDIR\imageformats\*.*"
  Delete "$INSTDIR\platforms\*.*"
  Delete "$INSTDIR\plugins\*.*"
  Delete "$INSTDIR\printsupport\*.*"
  Delete "$INSTDIR\sqldrivers\*.*"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.ico"
  Delete "$INSTDIR\*.dat"
  Delete "$INSTDIR\*.pak"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Starc\*.*"
  Delete "$DESKTOP\Story Architect.lnk"

  ; Remove directories used
  RMDir /r "$INSTDIR\audio\*.*"
  RMDir /r "$INSTDIR\imageformats\*.*"
  RMDir /r "$INSTDIR\platforms\*.*"
  RMDir /r "$INSTDIR\plugins\*.*"
  RMDir /r "$INSTDIR\printsupport\*.*"
  RMDir /r "$INSTDIR\sqldrivers\*.*"
  RMDir "$INSTDIR"
  RMDir "$SMPROGRAMS\Starc"
  
  ; Отменяем зарегистрированные ассоциации файлов
  ${unregisterExtension} ".starc" "Story Architect project"

SectionEnd

Function .onInit

  InitPluginsDir
  !insertmacro MUI_LANGDLL_DISPLAY

  IfSilent 0 +5
  CreateDirectory "$SMPROGRAMS\Starc"
  CreateShortCut "$SMPROGRAMS\Starc\Story Architect.lnk" "$INSTDIR\starcapp.exe"
  CreateShortCut "$SMPROGRAMS\Starc\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$DESKTOP\Story Architect.lnk" "$INSTDIR\starcapp.exe" "" "$INSTDIR\starcapp.exe" 0
	
FunctionEnd

