TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = $$qtLibraryTarget(screenplayinformationplugin)

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
    screenplay_information_manager.h \
    screenplay_information_view.h

SOURCES += \
    screenplay_information_manager.cpp \
    screenplay_information_view.cpp
