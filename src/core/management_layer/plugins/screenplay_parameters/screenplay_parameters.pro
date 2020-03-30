TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = screenplayparametersplugin

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
    screenplay_parameters_manager.h \
    screenplay_parameters_view.h

SOURCES += \
    screenplay_parameters_manager.cpp \
    screenplay_parameters_view.cpp
