TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = screenplaytextstructureplugin

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
    business_layer/screenplay_text_structure_model.h \
    screenplay_text_structure_manager.h \
    ui/screenplay_text_structure_view.h

SOURCES += \
    business_layer/screenplay_text_structure_model.cpp \
    screenplay_text_structure_manager.cpp \
    ui/screenplay_text_structure_view.cpp
