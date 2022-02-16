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
EXE_APP_DIR="files_64"
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
cp $Qt5_Dir/plugins/audio/*.dll $EXE_APP_DIR/audio/
mkdir $EXE_APP_DIR/iconengines
cp $Qt5_Dir/plugins/iconengines/*.dll $EXE_APP_DIR/iconengines/
mkdir $EXE_APP_DIR/imageformats
cp $Qt5_Dir/plugins/imageformats/*.dll $EXE_APP_DIR/imageformats/
mkdir $EXE_APP_DIR/platforms
cp $Qt5_Dir/plugins/platforms/qwindows.dll $EXE_APP_DIR/platforms/
mkdir $EXE_APP_DIR/printsupport
cp $Qt5_Dir/plugins/printsupport/*.dll $EXE_APP_DIR/printsupport/
mkdir $EXE_APP_DIR/sqldrivers
cp $Qt5_Dir/plugins/sqldrivers/qsqlite.dll $EXE_APP_DIR/sqldrivers/
#
# Copy openssl lib
#
cp /c/Program\ Files/OpenSSL-Win64/*.dll $EXE_APP_DIR
#
# Copy msvc lib
#
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/msvcp140.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/msvcp140_1.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/vcruntime140.dll" $EXE_APP_DIR
