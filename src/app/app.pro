TEMPLATE = app
TARGET = starcapp

CONFIG += c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += core gui widgets network

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build/

INCLUDEPATH += ..

LIBS += -L$$DESTDIR

SOURCES += \
        application.cpp \
        main.cpp

HEADERS += \
    application.h \
    ../include/deep_link.h

win32:RC_FILE = app.rc
macx {
    ICON = icon.icns
    QMAKE_INFO_PLIST = Info.plist
}
