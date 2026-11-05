TEMPLATE = app
TARGET = tst_deep_links
CONFIG += console testcase c++17
QT += core gui widgets network testlib
INCLUDEPATH += ../../src
SOURCES += tst_deep_links.cpp ../../src/app/application.cpp
HEADERS += ../../src/app/application.h
