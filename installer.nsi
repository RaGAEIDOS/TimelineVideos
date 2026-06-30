; TimelineVideo Windows Installer (NSIS)
Unicode True
RequestExecutionLevel admin

!define PRODUCT_NAME "TimelineVideo"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "TimelineVideo"
!define PRODUCT_WEB_SITE "https://github.com/RaGAEIDOS/TimelineVideos"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\TimelineVideo.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "TimelineVideo-Setup.exe"
InstallDir "$PROGRAMFILES64\TimelineVideo"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  File "build\TimelineVideo.exe"
  File "Img\logo.ico"
  File "resources.qrc"

  SetOutPath "$INSTDIR\locales"
  File "locales\ar.json"
  File "locales\en.json"

  SetOutPath "$INSTDIR\data"
  File /r "data\*.db"

  SetOutPath "$INSTDIR"
  CreateDirectory "$INSTDIR\data\thumbs"

  ; Start menu shortcut
  CreateDirectory "$SMPROGRAMS\TimelineVideo"
  CreateShortCut "$SMPROGRAMS\TimelineVideo\TimelineVideo.lnk" "$INSTDIR\TimelineVideo.exe" "" "$INSTDIR\logo.ico" 0
  CreateShortCut "$SMPROGRAMS\TimelineVideo\Uninstall.lnk" "$INSTDIR\uninst.exe"

  ; Desktop shortcut
  CreateShortCut "$DESKTOP\TimelineVideo.lnk" "$INSTDIR\TimelineVideo.exe" "" "$INSTDIR\logo.ico" 0
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\TimelineVideo.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
SectionEnd

Section Uninstall
  Delete "$INSTDIR\TimelineVideo.exe"
  Delete "$INSTDIR\logo.ico"
  Delete "$INSTDIR\resources.qrc"
  Delete "$INSTDIR\locales\ar.json"
  Delete "$INSTDIR\locales\en.json"
  Delete "$INSTDIR\data\*"
  Delete "$INSTDIR\uninst.exe"
  RMDir /r "$INSTDIR\data"
  RMDir "$INSTDIR\locales"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\TimelineVideo\TimelineVideo.lnk"
  Delete "$SMPROGRAMS\TimelineVideo\Uninstall.lnk"
  RMDir "$SMPROGRAMS\TimelineVideo"
  Delete "$DESKTOP\TimelineVideo.lnk"

  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
SectionEnd
