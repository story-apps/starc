TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = $$qtLibraryTarget(projectinformationplugin)

DEFINES += VIEW_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../../../../../_build/plugins

INCLUDEPATH += $$PWD/../../../../..

#
# Подключаем библиотеку corelib
#

LIBS += -L$$DESTDIR/../ -lcorelib
INCLUDEPATH += $$PWD/../../../../../corelib
DEPENDPATH += $$PWD/../../../../../corelib

#

HEADERS += \
    cover.h \
    project_information_view.h \
    project_information_view_plugin.h

SOURCES += \
    cover.cpp \
    project_information_view.cpp \
    project_information_view_plugin.cpp
