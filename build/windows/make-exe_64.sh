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
cp $Qt6_DIR/bin/d3dcompiler_47.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/opengl32sw.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Concurrent.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Core.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Core5Compat.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6DBus.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Gui.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Multimedia.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Network.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6PrintSupport.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Sql.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Svg.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6WebSockets.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Widgets.dll $EXE_APP_DIR/
cp $Qt6_DIR/bin/Qt6Xml.dll $EXE_APP_DIR/
#
# Copy Qt plugins
#
mkdir $EXE_APP_DIR/generic
cp $Qt6_DIR/plugins/generic/qtuiotouchplugin.dll $EXE_APP_DIR/audio/
mkdir $EXE_APP_DIR/iconengines
cp $Qt6_DIR/plugins/iconengines/qsvgicon.dll $EXE_APP_DIR/iconengines/
mkdir $EXE_APP_DIR/imageformats
cp $Qt6_DIR/plugins/imageformats/qgif.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qicns.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qico.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qjpeg.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qsvg.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qtga.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qtiff.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qwbmp.dll $EXE_APP_DIR/imageformats/
cp $Qt6_DIR/plugins/imageformats/qwebp.dll $EXE_APP_DIR/imageformats/
mkdir $EXE_APP_DIR/multimedia
cp $Qt6_DIR/plugins/multimedia/ffmpegmediaplugin.dll $EXE_APP_DIR/multimedia/
cp $Qt6_DIR/plugins/multimedia/windowsmediaplugin.dll $EXE_APP_DIR/multimedia/
mkdir $EXE_APP_DIR/networkinformation
cp $Qt6_DIR/plugins/networkinformation/qnetworklistmanager.dll $EXE_APP_DIR/networkinformation/
mkdir $EXE_APP_DIR/platforms
cp $Qt6_DIR/plugins/platforms/qwindows.dll $EXE_APP_DIR/platforms/
mkdir $EXE_APP_DIR/sqldrivers
cp $Qt6_DIR/plugins/sqldrivers/qsqlite.dll $EXE_APP_DIR/sqldrivers/
mkdir $EXE_APP_DIR/styles
cp $Qt6_DIR/plugins/styles/qmodernwindowsstyle.dll $EXE_APP_DIR/styles/
mkdir $EXE_APP_DIR/tls
cp $Qt6_DIR/plugins/tls/qcertonlybackend.dll $EXE_APP_DIR/tls/
cp $Qt6_DIR/plugins/tls/qopensslbackend.dll $EXE_APP_DIR/tls/
cp $Qt6_DIR/plugins/tls/qschannelbackend.dll $EXE_APP_DIR/tls/
#
# Copy openssl lib
#
cp /c/Program\ Files/OpenSSL/*.dll $EXE_APP_DIR
#
# Copy msvc lib
#
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/msvcp140.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/msvcp140_1.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/msvcp140_2.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/vcruntime140.dll" $EXE_APP_DIR
cp "$VCToolsRedistDir/x64/Microsoft.VC143.CRT/vcruntime140_1.dll" $EXE_APP_DIR
