#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:03:13
#
#-------------------------------------------------

TEMPLATE = app
TARGET = starcapp

CONFIG += c++1z
QT += core gui widgets

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build/

INCLUDEPATH += ..

LIBS += -L$$DESTDIR

SOURCES += \
        application.cpp \
        main.cpp

HEADERS += \
    application.h

win32:RC_FILE = app.rc
macx {
    ICON = icon.icns
    QMAKE_INFO_PLIST = Info.plist
}
