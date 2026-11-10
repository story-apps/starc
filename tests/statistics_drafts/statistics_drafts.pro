TEMPLATE = app
TARGET = tst_statistics_drafts
CONFIG += testcase c++17
QT += widgets testlib sql xml

INCLUDEPATH += $$PWD/../../src $$PWD/../../src/corelib $$PWD/../../src/core
STARC_BUILD = $$clean_path($$PWD/../../src/_build)
mac {
    CORELIB_DIR = $$STARC_BUILD/starcapp.app/Contents/Frameworks
} else {
    CORELIB_DIR = $$STARC_BUILD
}
LIBS += -L$$CORELIB_DIR -lcorelib
QMAKE_RPATHDIR += $$CORELIB_DIR $$STARC_BUILD/libs
SOURCES += tst_statistics_drafts.cpp \
    ../../src/core/management_layer/content/project/project_models_facade.cpp
HEADERS += ../../src/core/management_layer/content/project/project_models_facade.h

contains(CONFIG, statistics_plugins) {
    DEFINES += TEST_STATISTICS_PLUGINS
}
