TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++1z

TARGET = crashpad_paths

QT += core

# Output directory
DESTDIR = ../../_build/libs

# Source files
SOURCES += \
    crashpad_paths.cpp

HEADERS += \
    crashpad_paths.h
