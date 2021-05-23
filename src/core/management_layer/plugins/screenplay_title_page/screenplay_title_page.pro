TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = screenplaytitlepageplugin

DEFINES += MANAGER_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

mac {
    DESTDIR = ../../../../_build/starcapp.app/Contents/PlugIns
    CORELIBDIR = ../../../../_build/starcapp.app/Contents/Frameworks
} else {
    DESTDIR = ../../../../_build/plugins
    CORELIBDIR = ../../../../_build
}

INCLUDEPATH += $$PWD/../../../..

#
# Подключаем библиотеку corelib
#

LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../../../../corelib
DEPENDPATH += $$PWD/../../../../corelib

#

HEADERS += \
    screenplay_title_page_manager.h \
    screenplay_title_page_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/standard_key_handler.h \
    text/handlers/text_handler.h \
    text/screenplay_title_page_edit.h \
    text/screenplay_title_page_edit_toolbar.h

SOURCES += \
    screenplay_title_page_manager.cpp \
    screenplay_title_page_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/handlers/text_handler.cpp \
    text/screenplay_title_page_edit.cpp \
    text/screenplay_title_page_edit_toolbar.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
