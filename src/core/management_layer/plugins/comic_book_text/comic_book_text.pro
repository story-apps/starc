TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets

TARGET = comicbooktextplugin

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
    comments/comic_book_text_add_comment_view.h \
    comments/comic_book_text_comment_delegate.h \
    comments/comic_book_text_comment_replies_view.h \
    comments/comic_book_text_comment_view.h \
    comments/comic_book_text_comments_model.h \
    comments/comic_book_text_comments_toolbar.h \
    comments/comic_book_text_comments_view.h \
    comic_book_text_manager.h \
    comic_book_text_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/character_handler.h \
    text/handlers/description_handler.h \
    text/handlers/dialog_handler.h \
    text/handlers/folder_footer_handler.h \
    text/handlers/folder_header_handler.h \
    text/handlers/inline_note_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/page_handler.h \
    text/handlers/panel_handler.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/standard_key_handler.h \
    text/handlers/unformatted_text_handler.h \
    text/comic_book_text_edit.h \
    text/comic_book_text_edit_shortcuts_manager.h \
    text/comic_book_text_edit_toolbar.h \
    text/comic_book_text_fast_format_widget.h \
    text/comic_book_text_search_manager.h \
    text/comic_book_text_search_toolbar.h

SOURCES += \
    comments/comic_book_text_add_comment_view.cpp \
    comments/comic_book_text_comment_delegate.cpp \
    comments/comic_book_text_comment_replies_view.cpp \
    comments/comic_book_text_comment_view.cpp \
    comments/comic_book_text_comments_toolbar.cpp \
    comments/comic_book_text_comments_model.cpp \
    comments/comic_book_text_comments_view.cpp \
    comic_book_text_manager.cpp \
    comic_book_text_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/character_handler.cpp \
    text/handlers/description_handler.cpp \
    text/handlers/dialog_handler.cpp \
    text/handlers/folder_footer_handler.cpp \
    text/handlers/folder_header_handler.cpp \
    text/handlers/inline_note_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/page_handler.cpp \
    text/handlers/panel_handler.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/handlers/unformatted_text_handler.cpp \
    text/comic_book_text_edit.cpp \
    text/comic_book_text_edit_shortcuts_manager.cpp \
    text/comic_book_text_edit_toolbar.cpp \
    text/comic_book_text_fast_format_widget.cpp \
    text/comic_book_text_search_manager.cpp \
    text/comic_book_text_search_toolbar.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
