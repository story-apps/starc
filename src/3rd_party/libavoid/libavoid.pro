TEMPLATE = lib
TARGET = libavoid

CONFIG += staticlib

QT -= core gui

#
# Конфигурируем расположение файлов сборки
#
DESTDIR = ../../_build/libs

# Input
HEADERS += \
    libavoid/actioninfo.h \
    libavoid/assertions.h \
    libavoid/connectionpin.h \
    libavoid/connector.h \
    libavoid/connend.h \
    libavoid/debug.h \
    libavoid/debughandler.h \
    libavoid/dllexport.h \
    libavoid/geometry.h \
    libavoid/geomtypes.h \
    libavoid/graph.h \
    libavoid/hyperedge.h \
    libavoid/hyperedgeimprover.h \
    libavoid/hyperedgetree.h \
    libavoid/junction.h \
    libavoid/libavoid.h \
    libavoid/makepath.h \
    libavoid/mtst.h \
    libavoid/obstacle.h \
    libavoid/orthogonal.h \
    libavoid/router.h \
    libavoid/scanline.h \
    libavoid/shape.h \
    libavoid/timer.h \
    libavoid/vertices.h \
    libavoid/viscluster.h \
    libavoid/visibility.h \
    libavoid/vpsc.h

SOURCES += \
    libavoid/actioninfo.cpp \
    libavoid/connectionpin.cpp \
    libavoid/connector.cpp \
    libavoid/connend.cpp \
    libavoid/geometry.cpp \
    libavoid/geomtypes.cpp \
    libavoid/graph.cpp \
    libavoid/hyperedge.cpp \
    libavoid/hyperedgeimprover.cpp \
    libavoid/hyperedgetree.cpp \
    libavoid/junction.cpp \
    libavoid/makepath.cpp \
    libavoid/mtst.cpp \
    libavoid/obstacle.cpp \
    libavoid/orthogonal.cpp \
    libavoid/router.cpp \
    libavoid/scanline.cpp \
    libavoid/shape.cpp \
    libavoid/timer.cpp \
    libavoid/vertices.cpp \
    libavoid/viscluster.cpp \
    libavoid/visibility.cpp \
    libavoid/vpsc.cpp
