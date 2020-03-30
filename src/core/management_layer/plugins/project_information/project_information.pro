TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = projectinformationplugin

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
    cover.h \
    cover_dialog.h \
    project_information_manager.h \
    project_information_view.h

SOURCES += \
    cover.cpp \
    cover_dialog.cpp \
    project_information_manager.cpp \
    project_information_view.cpp
