#!/bin/sh

#
# $1 - application version
#

#
# Prepare folders structure
#
APP_IMAGE_DIR="appdir"
mkdir $APP_IMAGE_DIR/lib
mkdir $APP_IMAGE_DIR/plugins
ls -l $APP_IMAGE_DIR

#
# Copy application binaries
#
APP_BIN_DIR="../../src/_build"
ls -l $APP_BIN_DIR
cp $APP_BIN_DIR/starcapi $APP_IMAGE_DIR/starc
cp $APP_BIN_DIR/libcorelib.so.1 $APP_IMAGE_DIR/lib/

#
# Make installer
#
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O linuxdeployqt
chmod a+x linuxdeployqt
./linuxdeployqt appdir/starc -no-copy-copyright-files -no-translations -always-overwrite -executable=appdir/plugins/libcoreplugin.so -appimage
mv *.AppImage starc-api-$1.AppImage
