TARGET = qgumboparser
TEMPLATE = lib

QT -= gui

CONFIG += staticlib c++11
QMAKE_CFLAGS += -std=c99

#
# Конфигурируем расположение файлов сборки
#
DESTDIR = ../../_build/libs

SOURCES += \
    qgumboattribute.cpp \
    qgumbodocument.cpp \
    qgumbonode.cpp \
    gumbo-parser/src/attribute.c \
    gumbo-parser/src/char_ref.c \
    gumbo-parser/src/error.c \
    gumbo-parser/src/parser.c \
    gumbo-parser/src/string_buffer.c \
    gumbo-parser/src/string_piece.c \
    gumbo-parser/src/tag.c \
    gumbo-parser/src/tokenizer.c \
    gumbo-parser/src/utf8.c \
    gumbo-parser/src/util.c \
    gumbo-parser/src/vector.c

HEADERS += \
    qgumboattribute.h \
    qgumbodocument.h \
    qgumbonode.h \
    gumbo-parser/src/attribute.h \
    gumbo-parser/src/char_ref.h \
    gumbo-parser/src/error.h \
    gumbo-parser/src/gumbo.h \
    gumbo-parser/src/insertion_mode.h \
    gumbo-parser/src/parser.h \
    gumbo-parser/src/string_buffer.h \
    gumbo-parser/src/string_piece.h \
    gumbo-parser/src/token_type.h \
    gumbo-parser/src/tokenizer_states.h \
    gumbo-parser/src/tokenizer.h \
    gumbo-parser/src/utf8.h \
    gumbo-parser/src/util.h \
    gumbo-parser/src/vector.h \
    HtmlTag.h \
    gumbo-parser/visualc/include/strings.h \
    helper.h

contains(QMAKE_CC, gcc): {
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration
}
