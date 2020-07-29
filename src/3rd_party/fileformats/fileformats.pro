TARGET   = fileformats
TEMPLATE = lib

DEFINES += FILEFORMATS_LIBRARY

#
# Build configuration
#
CONFIG += qt thread warn_on staticlib

#
# Конфигурируем расположение файлов сборки
#
DESTDIR = ../../_build/libs

mac {
     LIBS += -lz
}

HEADERS += \
    docx_reader.h \
    docx_writer.h \
    format_manager.h \
    format_reader.h \
    odt_reader.h \
    rtf_reader.h \
    rtf_tokenizer.h \
    txt_reader.h \
    qtzip/qtzipreader.h \
    qtzip/QtZipReader \
    qtzip/qtzipwriter.h \
    qtzip/QtZipWriter \
    format_helpers.h

SOURCES += \
    docx_reader.cpp \
    docx_writer.cpp \
    format_manager.cpp \
    odt_reader.cpp \
    rtf_reader.cpp \
    rtf_tokenizer.cpp \
    txt_reader.cpp \
    qtzip/qtzip.cpp
