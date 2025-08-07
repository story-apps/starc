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

#
# Copy application binaries
#
APP_BIN_DIR="../../src/_build"
ls -l $APP_BIN_DIR
cp $APP_BIN_DIR/starcapp.exe $EXE_APP_DIR
cp $APP_BIN_DIR/corelib.dll $EXE_APP_DIR
cp $APP_BIN_DIR/plugins/*.dll $EXE_APP_DIR/plugins/

#
# Copy Qt libraries
#
cp $Qt5_Dir/bin/d3dcompiler_47.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/libEGL.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/libGLESv2.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/opengl32sw.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Concurrent.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Core.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5DBus.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Gui.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Multimedia.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Network.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5PrintSupport.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Sql.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Svg.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5WebSockets.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Widgets.dll $EXE_APP_DIR/
cp $Qt5_Dir/bin/Qt5Xml.dll $EXE_APP_DIR/
#
# Copy Qt plugins
#
mkdir $EXE_APP_DIR/audio
cp $Qt5_Dir/plugins/audio/qtaudio_wasapi.dll $EXE_APP_DIR/audio/
cp $Qt5_Dir/plugins/audio/qtaudio_windows.dll $EXE_APP_DIR/audio/
mkdir $EXE_APP_DIR/iconengines
cp $Qt5_Dir/plugins/iconengines/qsvgicon.dll $EXE_APP_DIR/iconengines/
mkdir $EXE_APP_DIR/imageformats
cp $Qt5_Dir/plugins/imageformats/qgif.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qicns.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qico.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qjpeg.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qsvg.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qtga.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qtiff.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qwbmp.dll $EXE_APP_DIR/imageformats/
cp $Qt5_Dir/plugins/imageformats/qwebp.dll $EXE_APP_DIR/imageformats/
mkdir $EXE_APP_DIR/platforms
cp $Qt5_Dir/plugins/platforms/qwindows.dll $EXE_APP_DIR/platforms/
mkdir $EXE_APP_DIR/printsupport
cp $Qt5_Dir/plugins/printsupport/windowsprintersupport.dll $EXE_APP_DIR/printsupport/
mkdir $EXE_APP_DIR/sqldrivers
cp $Qt5_Dir/plugins/sqldrivers/qsqlite.dll $EXE_APP_DIR/sqldrivers/
mkdir $EXE_APP_DIR/styles
cp $Qt5_Dir/plugins/styles/qwindowsvistastyle.dll $EXE_APP_DIR/styles/
#
# Copy openssl lib
#
cp /c/Program\ Files\ \(x86\)/OpenSSL-Win32/*.dll $EXE_APP_DIR
#
# Copy msvc lib
#
cp "$VCToolsRedistDir/x86/Microsoft.VC143.CRT/msvcp140.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x86/Microsoft.VC143.CRT/msvcp140_1.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x86/Microsoft.VC143.CRT/vcruntime140.dll" $EXE_APP_DIR
