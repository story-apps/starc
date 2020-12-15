TEMPLATE = app
TARGET = testapp

CONFIG += c++11
QT += core gui widgets

DESTDIR = ../_build/

#
# Подключаем библиотеку corelib
#
LIBS += -L$$DESTDIR/ -lcorelib
INCLUDEPATH += $$PWD/../corelib
DEPENDPATH += $$PWD/../corelib

SOURCES += \
    main.cpp
