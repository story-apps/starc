TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = textplugin

DEFINES += MANAGER_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../../../../_build/plugins

INCLUDEPATH += $$PWD/../../../..

#
# Подключаем библиотеку corelib
#

LIBS += -L$$DESTDIR/../ -lcorelib
INCLUDEPATH += $$PWD/../../../../corelib
DEPENDPATH += $$PWD/../../../../corelib

#

HEADERS += \
    text_manager.h \
    text_view.h

SOURCES += \
    text_manager.cpp \
    text_view.cpp
