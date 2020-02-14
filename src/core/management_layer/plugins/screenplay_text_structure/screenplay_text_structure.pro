TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = $$qtLibraryTarget(screenplaytextstructureplugin)

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
    screenplay_text_structure_manager.h \
    ui/screenplay_text_structure_view.h

SOURCES += \
    screenplay_text_structure_manager.cpp \
    ui/screenplay_text_structure_view.cpp
