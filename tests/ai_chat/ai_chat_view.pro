TEMPLATE = app
TARGET = tst_ai_chat_view
CONFIG += testcase c++17
QT += widgets sql testlib
INCLUDEPATH += ../../src/corelib ../../src/core ../../src
STARC_BUILD = $$clean_path($$PWD/../../src/_build)
mac {
    CORELIB_DIR = $$STARC_BUILD/starcapp.app/Contents/Frameworks
} else {
    CORELIB_DIR = $$STARC_BUILD
}
LIBS += -L$$CORELIB_DIR -lcorelib
QMAKE_RPATHDIR += $$CORELIB_DIR $$STARC_BUILD/libs
SOURCES += tst_ai_chat_view.cpp \
    ../../src/core/management_layer/content/writing_session/writing_session_storage.cpp
