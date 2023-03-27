TEMPLATE = lib

CONFIG += plugin c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += widgets

TARGET = noveltextplugin

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
    novel_text_manager.h \
    novel_text_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/beat_heading_handler.h \
    text/handlers/chapter_footer_handler.h \
    text/handlers/chapter_heading_handler.h \
    text/handlers/inline_note_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/part_footer_handler.h \
    text/handlers/part_heading_handler.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/scene_heading_handler.h \
    text/handlers/standard_key_handler.h \
    text/handlers/text_handler.h \
    text/handlers/unformatted_text_handler.h \
    text/novel_text_edit.h \
    text/novel_text_edit_shortcuts_manager.h \
    text/novel_text_edit_toolbar.h \
    text/novel_text_search_manager.h \
    text/novel_text_search_toolbar.h \

SOURCES += \
    novel_text_manager.cpp \
    novel_text_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/beat_heading_handler.cpp \
    text/handlers/chapter_footer_handler.cpp \
    text/handlers/chapter_heading_handler.cpp \
    text/handlers/inline_note_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/part_footer_handler.cpp \
    text/handlers/part_heading_handler.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/scene_heading_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/handlers/text_handler.cpp \
    text/handlers/unformatted_text_handler.cpp \
    text/novel_text_edit.cpp \
    text/novel_text_edit_shortcuts_manager.cpp \
    text/novel_text_edit_toolbar.cpp \
    text/novel_text_search_manager.cpp \
    text/novel_text_search_toolbar.cpp \

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
