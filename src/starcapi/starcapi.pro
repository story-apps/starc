TARGET = starcapi

CONFIG += c++1z console
CONFIG -= app_bundle
QT += core gui widgets widgets-private

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
#

SOURCES += \
        main.cpp

DISTFILES += \
    starcapi.py
