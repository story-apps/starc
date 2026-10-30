TEMPLATE = app
TARGET = tst_remote_text_changes
CONFIG += testcase c++17
QT += widgets testlib

INCLUDEPATH += $$PWD/../../src $$PWD/../../src/corelib $$PWD/../../src/core
STARC_BUILD = $$clean_path($$PWD/../../src/_build)
mac {
    CORELIB_DIR = $$STARC_BUILD/starcapp.app/Contents/Frameworks
    PLUGINS_DIR = $$STARC_BUILD/starcapp.app/Contents/PlugIns
} else {
    CORELIB_DIR = $$STARC_BUILD
    PLUGINS_DIR = $$STARC_BUILD/plugins
}
LIBS += -L$$PLUGINS_DIR -lscreenplaytextplugin -L$$CORELIB_DIR -lcorelib
QMAKE_RPATHDIR += $$CORELIB_DIR $$PLUGINS_DIR $$STARC_BUILD/libs
SOURCES += tst_remote_text_changes.cpp
