TEMPLATE = app
TARGET = tst_ai_chat
CONFIG += testcase c++17
QT += core sql testlib concurrent
QT -= gui
DEFINES += CORE_LIBRARY
INCLUDEPATH += ../../src/corelib
SOURCES += tst_ai_chat.cpp \
    ../../src/corelib/ui/modules/ai_assistant/ai_project_context.cpp \
    ../../src/corelib/ui/modules/ai_assistant/ai_chat_session.cpp
HEADERS += ../../src/corelib/ui/modules/ai_assistant/ai_project_context.h \
    ../../src/corelib/ui/modules/ai_assistant/ai_chat_session.h
