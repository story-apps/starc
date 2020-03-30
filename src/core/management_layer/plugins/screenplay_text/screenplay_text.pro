TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = screenplaytextplugin

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
    screenplay_text_manager.h \
    ui/handlers/abstract_key_handler.h \
    ui/handlers/action_handler.h \
    ui/handlers/character_handler.h \
    ui/handlers/dialog_handler.h \
    ui/handlers/folder_footer_handler.h \
    ui/handlers/folder_header_handler.h \
    ui/handlers/inline_note_handler.h \
    ui/handlers/key_press_handler_facade.h \
    ui/handlers/lyrics_handler.h \
    ui/handlers/parenthetical_handler.h \
    ui/handlers/pre_handler.h \
    ui/handlers/prepare_handler.h \
    ui/handlers/scene_characters_handler.h \
    ui/handlers/scene_heading_handler.h \
    ui/handlers/shot_handler.h \
    ui/handlers/standard_key_handler.h \
    ui/handlers/transition_handler.h \
    ui/handlers/unformatted_text_handler.h \
    ui/screenplay_text_block_data.h \
    ui/screenplay_text_cursor.h \
    ui/screenplay_text_document.h \
    ui/screenplay_text_edit.h \
    ui/screenplay_text_edit_toolbar.h \
    ui/screenplay_text_view.h

SOURCES += \
    screenplay_text_manager.cpp \
    ui/handlers/abstract_key_handler.cpp \
    ui/handlers/action_handler.cpp \
    ui/handlers/character_handler.cpp \
    ui/handlers/dialog_handler.cpp \
    ui/handlers/folder_footer_handler.cpp \
    ui/handlers/folder_header_handler.cpp \
    ui/handlers/inline_note_handler.cpp \
    ui/handlers/key_press_handler_facade.cpp \
    ui/handlers/lyrics_handler.cpp \
    ui/handlers/parenthetical_handler.cpp \
    ui/handlers/pre_handler.cpp \
    ui/handlers/prepare_handler.cpp \
    ui/handlers/scene_characters_handler.cpp \
    ui/handlers/scene_heading_handler.cpp \
    ui/handlers/shot_handler.cpp \
    ui/handlers/standard_key_handler.cpp \
    ui/handlers/transition_handler.cpp \
    ui/handlers/unformatted_text_handler.cpp \
    ui/screenplay_text_block_data.cpp \
    ui/screenplay_text_cursor.cpp \
    ui/screenplay_text_document.cpp \
    ui/screenplay_text_edit.cpp \
    ui/screenplay_text_edit_toolbar.cpp \
    ui/screenplay_text_view.cpp

RESOURCES += \
    resources.qrc
