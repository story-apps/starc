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
DEB_APP_DIR="deb/usr/share/starc"
mkdir $DEB_APP_DIR/libs
mkdir $DEB_APP_DIR/plugins
ls -l $DEB_APP_DIR

#
# Copy application binaries
#
APP_BIN_DIR="../../src/_build"
ls -l $APP_BIN_DIR
cp $APP_BIN_DIR/starcapp $DEB_APP_DIR/starc
cp $APP_BIN_DIR/libcorelib.so.1 $DEB_APP_DIR/libs/
cp $APP_BIN_DIR/plugins/*.so $DEB_APP_DIR/plugins/

#
# Copy Qt libraries
#
cp $Qt5_DIR/lib/libicudata.so.56 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libicui18n.so.56 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libicuuc.so.56 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Concurrent.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Core.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5DBus.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Gui.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Multimedia.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Network.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5PrintSupport.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Sql.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Svg.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5WebSockets.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Widgets.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5XcbQpa.so.5 $DEB_APP_DIR/libs/
cp $Qt5_DIR/lib/libQt5Xml.so.5 $DEB_APP_DIR/libs/
#
# Copy Qt plugins
#
mkdir $DEB_APP_DIR/plugins/audio
cp $Qt5_DIR/plugins/audio/*.so $DEB_APP_DIR/plugins/audio/
mkdir $DEB_APP_DIR/plugins/iconengines
cp $Qt5_DIR/plugins/iconengines/*.so $DEB_APP_DIR/plugins/iconengines/
mkdir $DEB_APP_DIR/plugins/imageformats
cp $Qt5_DIR/plugins/imageformats/*.so $DEB_APP_DIR/plugins/imageformats/
mkdir $DEB_APP_DIR/plugins/platforminputcontexts
cp $Qt5_DIR/plugins/platforminputcontexts/*.so $DEB_APP_DIR/plugins/platforminputcontexts/
mkdir $DEB_APP_DIR/plugins/platforms
cp $Qt5_DIR/plugins/platforms/libqxcb.so $DEB_APP_DIR/plugins/platforms/
mkdir $DEB_APP_DIR/plugins/printsupport
cp $Qt5_DIR/plugins/printsupport/*.so $DEB_APP_DIR/plugins/printsupport/
mkdir $DEB_APP_DIR/plugins/sqldrivers
cp $Qt5_DIR/plugins/sqldrivers/libqsqlite.so $DEB_APP_DIR/plugins/sqldrivers/

#
# Make installer
#
fakeroot dpkg-deb --build deb
mv -f deb.deb starc-setup-$1_amd64.deb
