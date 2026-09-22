#!/bin/sh

#
# TODO: 
# Add keys for architecture i386 or amd64
# Add version update availability
# Rename result file to choosing version and architecture
#

#
# $1 - application version
#

#
# Prepare folders structure
#
EXE_APP_DIR="files_32"
mkdir $EXE_APP_DIR
mkdir $EXE_APP_DIR/plugins
mkdir $EXE_APP_DIR/crashpad

#
# Copy application binaries
#
APP_BIN_DIR="../../src/_build"
cp $APP_BIN_DIR/starcapp.exe $EXE_APP_DIR
cp $APP_BIN_DIR/corelib.dll $EXE_APP_DIR
cp $APP_BIN_DIR/plugins/*.dll $EXE_APP_DIR/plugins/
cp $APP_BIN_DIR/crashpad/crashpad_handler.exe $EXE_APP_DIR/crashpad/

#
# Copy Qt libraries
#
cp $QT_ROOT_DIR/bin/d3dcompiler_47.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/libEGL.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/libGLESv2.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/opengl32sw.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Concurrent.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Core.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5DBus.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Gui.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Multimedia.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Network.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5PrintSupport.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Sql.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Svg.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5WebSockets.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Widgets.dll $EXE_APP_DIR/
cp $QT_ROOT_DIR/bin/Qt5Xml.dll $EXE_APP_DIR/
#
# Copy Qt plugins
#
mkdir $EXE_APP_DIR/audio
cp $QT_ROOT_DIR/plugins/audio/qtaudio_wasapi.dll $EXE_APP_DIR/audio/
cp $QT_ROOT_DIR/plugins/audio/qtaudio_windows.dll $EXE_APP_DIR/audio/
mkdir $EXE_APP_DIR/iconengines
cp $QT_ROOT_DIR/plugins/iconengines/qsvgicon.dll $EXE_APP_DIR/iconengines/
mkdir $EXE_APP_DIR/imageformats
cp $QT_ROOT_DIR/plugins/imageformats/qgif.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qicns.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qico.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qjpeg.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qsvg.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qtga.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qtiff.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qwbmp.dll $EXE_APP_DIR/imageformats/
cp $QT_ROOT_DIR/plugins/imageformats/qwebp.dll $EXE_APP_DIR/imageformats/
mkdir $EXE_APP_DIR/platforms
cp $QT_ROOT_DIR/plugins/platforms/qwindows.dll $EXE_APP_DIR/platforms/
mkdir $EXE_APP_DIR/printsupport
cp $QT_ROOT_DIR/plugins/printsupport/windowsprintersupport.dll $EXE_APP_DIR/printsupport/
mkdir $EXE_APP_DIR/sqldrivers
cp $QT_ROOT_DIR/plugins/sqldrivers/qsqlite.dll $EXE_APP_DIR/sqldrivers/
mkdir $EXE_APP_DIR/styles
cp $QT_ROOT_DIR/plugins/styles/qwindowsvistastyle.dll $EXE_APP_DIR/styles/
#
# Copy openssl lib
#
cp /c/OpenSSL32/bin/libcrypto*.dll $EXE_APP_DIR
cp /c/OpenSSL32/bin/libssl*.dll $EXE_APP_DIR
#
# Copy msvc lib
#
cp "$VCToolsRedistDir/x86/Microsoft.VC143.CRT/msvcp140.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x86/Microsoft.VC143.CRT/msvcp140_1.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x86/Microsoft.VC143.CRT/vcruntime140.dll" $EXE_APP_DIR
