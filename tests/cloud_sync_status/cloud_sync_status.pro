TEMPLATE = app
TARGET = tst_cloud_sync_status
CONFIG += testcase c++17
QT += widgets testlib
INCLUDEPATH += $$PWD/../../src/core $$PWD/../../src/corelib
STARC_BUILD = $$clean_path($$PWD/../../src/_build)
mac {
    CORELIB_DIR = $$STARC_BUILD/starcapp.app/Contents/Frameworks
} else {
    CORELIB_DIR = $$STARC_BUILD
}
LIBS += -L$$CORELIB_DIR -lcorelib
QMAKE_RPATHDIR += $$CORELIB_DIR $$STARC_BUILD/libs
SOURCES += tst_cloud_sync_status.cpp \
    ../../src/core/ui/account/connection_status_tool_bar.cpp
HEADERS += ../../src/core/ui/account/connection_status_tool_bar.h
