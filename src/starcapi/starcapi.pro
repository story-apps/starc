TARGET = starcapi

CONFIG += c++1z console
CONFIG -= app_bundle
QT += core gui widgets

DESTDIR = ../_build/

CORELIBDIR = ../_build
LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../corelib
DEPENDPATH += $$PWD/../corelib

SOURCES += \
        main.cpp

DISTFILES += \
    starcapi.py
