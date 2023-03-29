TEMPLATE = lib

CONFIG += plugin c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += widgets

TARGET = screenplaytreatmentplugin

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
    screenplay_treatment_manager.h \
    screenplay_treatment_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/act_footer_handler.h \
    text/handlers/act_heading_handler.h \
    text/handlers/beat_heading_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/scene_characters_handler.h \
    text/handlers/scene_heading_handler.h \
    text/handlers/sequence_footer_handler.h \
    text/handlers/sequence_heading_handler.h \
    text/handlers/standard_key_handler.h \
    text/screenplay_treatment_edit.h \
    text/screenplay_treatment_edit_shortcuts_manager.h \
    text/screenplay_treatment_edit_toolbar.h \
    text/screenplay_treatment_search_manager.h \
    text/screenplay_treatment_search_toolbar.h

SOURCES += \
    screenplay_treatment_manager.cpp \
    screenplay_treatment_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/act_footer_handler.cpp \
    text/handlers/act_heading_handler.cpp \
    text/handlers/beat_heading_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/scene_characters_handler.cpp \
    text/handlers/scene_heading_handler.cpp \
    text/handlers/sequence_footer_handler.cpp \
    text/handlers/sequence_heading_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/screenplay_treatment_edit.cpp \
    text/screenplay_treatment_edit_shortcuts_manager.cpp \
    text/screenplay_treatment_edit_toolbar.cpp \
    text/screenplay_treatment_search_manager.cpp \
    text/screenplay_treatment_search_toolbar.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
