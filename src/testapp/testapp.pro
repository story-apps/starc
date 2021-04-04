TEMPLATE = app
TARGET = testapp

CONFIG += c++11
QT += core gui widgets

DESTDIR = ../_build/

#
# Подключаем библиотеку corelib
#
mac {
    CORELIBDIR = ../_build/starcapp.app/Contents/Frameworks
} else {
    CORELIBDIR = ../_build
}
LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../corelib
DEPENDPATH += $$PWD/../corelib

SOURCES += \
    main.cpp
