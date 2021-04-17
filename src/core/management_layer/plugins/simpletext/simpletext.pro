TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = simpletextplugin

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
    simpletext_manager.h \
    simpletext_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/inline_note_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/scene_heading_handler.h \
    text/handlers/standard_key_handler.h \
    text/handlers/text_handler.h \
    text/text_edit.h \
    text/text_edit_shortcuts_manager.h \
    text/text_edit_toolbar.h \
    text/text_search_manager.h \
    text/text_search_toolbar.h

SOURCES += \
    simpletext_manager.cpp \
    simpletext_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/inline_note_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/scene_heading_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/handlers/text_handler.cpp \
    text/text_edit.cpp \
    text/text_edit_shortcuts_manager.cpp \
    text/text_edit_toolbar.cpp \
    text/text_search_manager.cpp \
    text/text_search_toolbar.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
