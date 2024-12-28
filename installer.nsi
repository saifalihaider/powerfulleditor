# Video Editor Installer Script
!include "MUI2.nsh"
!include "FileFunc.nsh"

# Basic definitions
!define PRODUCT_NAME "Professional Video Editor"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Your Company"
!define PRODUCT_WEB_SITE "https://www.yourcompany.com"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\VideoEditor.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

# Modern UI settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\modern-wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\modern-wizard.bmp"

# Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

# Uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Language
!insertmacro MUI_LANGUAGE "English"

# Installer attributes
OutFile "VideoEditorSetup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    SetOverwrite ifnewer
    
    # Main executable and dependencies
    File "..\build\Release\VideoEditor.exe"
    File "..\build\Release\*.dll"
    
    # CUDA dependencies
    File "..\build\Release\cuda\*.dll"
    
    # FFmpeg dependencies
    File "..\build\Release\ffmpeg\*.dll"
    
    # Qt dependencies
    File "..\build\Release\platforms\*.dll"
    File "..\build\Release\styles\*.dll"
    File "..\build\Release\imageformats\*.dll"
    
    # Resources
    CreateDirectory "$INSTDIR\resources"
    File /r "..\resources\*.*"
    
    # Configuration
    CreateDirectory "$INSTDIR\config"
    File "..\config\default.ini"
    
    # Create shortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\VideoEditor.lnk" "$INSTDIR\VideoEditor.exe"
    CreateShortCut "$DESKTOP\VideoEditor.lnk" "$INSTDIR\VideoEditor.exe"
    
    # Register file associations
    WriteRegStr HKCR ".mp4" "" "VideoEditor.VideoFile"
    WriteRegStr HKCR "VideoEditor.VideoFile" "" "Video File"
    WriteRegStr HKCR "VideoEditor.VideoFile\DefaultIcon" "" "$INSTDIR\VideoEditor.exe,0"
    WriteRegStr HKCR "VideoEditor.VideoFile\shell\open\command" "" '"$INSTDIR\VideoEditor.exe" "%1"'
    
    # Register application
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\VideoEditor.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    
    # Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
    # Remove shortcuts
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\VideoEditor.lnk"
    Delete "$DESKTOP\VideoEditor.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
    
    # Remove files
    Delete "$INSTDIR\VideoEditor.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\cuda\*.dll"
    Delete "$INSTDIR\ffmpeg\*.dll"
    Delete "$INSTDIR\platforms\*.dll"
    Delete "$INSTDIR\styles\*.dll"
    Delete "$INSTDIR\imageformats\*.dll"
    Delete "$INSTDIR\config\*.ini"
    RMDir /r "$INSTDIR\resources"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    
    # Remove registry entries
    DeleteRegKey HKCR ".mp4"
    DeleteRegKey HKCR "VideoEditor.VideoFile"
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
SectionEnd

Function .onInit
    # Check for previous installation
    ReadRegStr $R0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
    StrCmp $R0 "" done
    
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
        "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the previous version or `Cancel` to cancel this upgrade." \
        IDOK uninst
    Abort
    
uninst:
    ExecWait '$R0 _?=$INSTDIR'
    
done:
FunctionEnd
