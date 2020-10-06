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
    comments/screenplay_text_add_comment_widget.h \
    comments/screenplay_text_comment_delegate.h \
    comments/screenplay_text_comments_model.h \
    comments/screenplay_text_comments_toolbar.h \
    comments/screenplay_text_comments_view.h \
    screenplay_text_manager.h \
    screenplay_text_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/action_handler.h \
    text/handlers/character_handler.h \
    text/handlers/dialog_handler.h \
    text/handlers/folder_footer_handler.h \
    text/handlers/folder_header_handler.h \
    text/handlers/inline_note_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/lyrics_handler.h \
    text/handlers/parenthetical_handler.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/scene_characters_handler.h \
    text/handlers/scene_heading_handler.h \
    text/handlers/shot_handler.h \
    text/handlers/standard_key_handler.h \
    text/handlers/transition_handler.h \
    text/handlers/unformatted_text_handler.h \
    text/screenplay_text_block_data.h \
    text/screenplay_text_corrector.h \
    text/screenplay_text_cursor.h \
    text/screenplay_text_document.h \
    text/screenplay_text_edit.h \
    text/screenplay_text_edit_shortcuts_manager.h \
    text/screenplay_text_edit_toolbar.h \
    text/screenplay_text_fast_format_widget.h \
    text/screenplay_text_scrollbar_manager.h \
    text/screenplay_text_search_toolbar.h

SOURCES += \
    comments/screenplay_text_add_comment_widget.cpp \
    comments/screenplay_text_comment_delegate.cpp \
    comments/screenplay_text_comments_toolbar.cpp \
    comments/screenplay_text_comments_model.cpp \
    comments/screenplay_text_comments_view.cpp \
    screenplay_text_manager.cpp \
    screenplay_text_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/action_handler.cpp \
    text/handlers/character_handler.cpp \
    text/handlers/dialog_handler.cpp \
    text/handlers/folder_footer_handler.cpp \
    text/handlers/folder_header_handler.cpp \
    text/handlers/inline_note_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/lyrics_handler.cpp \
    text/handlers/parenthetical_handler.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/scene_characters_handler.cpp \
    text/handlers/scene_heading_handler.cpp \
    text/handlers/shot_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/handlers/transition_handler.cpp \
    text/handlers/unformatted_text_handler.cpp \
    text/screenplay_text_block_data.cpp \
    text/screenplay_text_corrector.cpp \
    text/screenplay_text_cursor.cpp \
    text/screenplay_text_document.cpp \
    text/screenplay_text_edit.cpp \
    text/screenplay_text_edit_shortcuts_manager.cpp \
    text/screenplay_text_edit_toolbar.cpp \
    text/screenplay_text_fast_format_widget.cpp \
    text/screenplay_text_scrollbar_manager.cpp \
    text/screenplay_text_search_toolbar.cpp
