#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:23:46
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin c++11
QT += widgets

TARGET = $$qtLibraryTarget(coreplugin)

DEFINES += CORE_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build/plugins

INCLUDEPATH += ..

SOURCES += \
        management_layer/application_manager.cpp

HEADERS += \
        core_global.h \
        management_layer/application_manager.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
