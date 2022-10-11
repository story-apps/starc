TEMPLATE = lib
TARGET = qtxlsxwriter

CONFIG += staticlib

QT += core gui gui-private

#
# Конфигурируем расположение файлов сборки
#
DESTDIR = ../../_build/libs

DEFINES += XLSX_NO_LIB

HEADERS += xlsxdocpropscore_p.h \
    xlsxdocpropsapp_p.h \
    xlsxrelationships_p.h \
    xlsxutility_p.h \
    xlsxsharedstrings_p.h \
    xlsxcontenttypes_p.h \
    xlsxtheme_p.h \
    xlsxformat.h \
    xlsxworkbook.h \
    xlsxstyles_p.h \
    xlsxabstractsheet.h \
    xlsxabstractsheet_p.h \
    xlsxworksheet.h \
    xlsxworksheet_p.h \
    xlsxchartsheet.h \
    xlsxchartsheet_p.h \
    xlsxzipwriter_p.h \
    xlsxworkbook_p.h \
    xlsxformat_p.h \
    xlsxglobal.h \
    xlsxdrawing_p.h \
    xlsxzipreader_p.h \
    xlsxdocument.h \
    xlsxdocument_p.h \
    xlsxcell.h \
    xlsxcell_p.h \
    xlsxdatavalidation.h \
    xlsxdatavalidation_p.h \
    xlsxcellreference.h \
    xlsxcellrange.h \
    xlsxrichstring_p.h \
    xlsxrichstring.h \
    xlsxconditionalformatting.h \
    xlsxconditionalformatting_p.h \
    xlsxcolor_p.h \
    xlsxnumformatparser_p.h \
    xlsxdrawinganchor_p.h \
    xlsxmediafile_p.h \
    xlsxabstractooxmlfile.h \
    xlsxabstractooxmlfile_p.h \
    xlsxchart.h \
    xlsxchart_p.h \
    xlsxsimpleooxmlfile_p.h \
    xlsxcellformula.h \
    xlsxcellformula_p.h

SOURCES += xlsxdocpropscore.cpp \
    xlsxdocpropsapp.cpp \
    xlsxrelationships.cpp \
    xlsxutility.cpp \
    xlsxsharedstrings.cpp \
    xlsxcontenttypes.cpp \
    xlsxtheme.cpp \
    xlsxformat.cpp \
    xlsxstyles.cpp \
    xlsxworkbook.cpp \
    xlsxabstractsheet.cpp \
    xlsxworksheet.cpp \
    xlsxchartsheet.cpp \
    xlsxzipwriter.cpp \
    xlsxdrawing.cpp \
    xlsxzipreader.cpp \
    xlsxdocument.cpp \
    xlsxcell.cpp \
    xlsxdatavalidation.cpp \
    xlsxcellreference.cpp \
    xlsxcellrange.cpp \
    xlsxrichstring.cpp \
    xlsxconditionalformatting.cpp \
    xlsxcolor.cpp \
    xlsxnumformatparser.cpp \
    xlsxdrawinganchor.cpp \
    xlsxmediafile.cpp \
    xlsxabstractooxmlfile.cpp \
    xlsxchart.cpp \
    xlsxsimpleooxmlfile.cpp \
    xlsxcellformula.cpp

