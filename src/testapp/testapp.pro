TEMPLATE = app
TARGET = testapp

CONFIG += c++11
QT += core gui widgets network

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

#
# Подключаем библиотеку Webloader
#
LIBSDIR = ../_build/libs
LIBS += -L$$LIBSDIR/ -lwebloader
INCLUDEPATH += $$PWD/../3rd_party/webloader/src
DEPENDPATH += $$PWD/../3rd_party/webloader
#

SOURCES += \
    main.cpp
