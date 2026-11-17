TEMPLATE = app
TARGET = tst_draft_comparison
CONFIG += testcase c++17
QT += widgets testlib sql xml

INCLUDEPATH += $$PWD/../../src $$PWD/../../src/corelib $$PWD/../../src/core
STARC_BUILD = $$clean_path($$PWD/../../src/_build)
mac {
    CORELIB_DIR = $$STARC_BUILD/starcapp.app/Contents/Frameworks
    PLUGINS_DIR = $$STARC_BUILD/starcapp.app/Contents/PlugIns
} else {
    CORELIB_DIR = $$STARC_BUILD
    PLUGINS_DIR = $$STARC_BUILD/plugins
}
LIBS += -L$$PLUGINS_DIR -lsimpletextplugin -L$$CORELIB_DIR -lcorelib
QMAKE_RPATHDIR += $$CORELIB_DIR $$PLUGINS_DIR $$STARC_BUILD/libs
SOURCES += tst_draft_comparison.cpp \
    ../../src/core/management_layer/content/project/project_models_facade.cpp \
    ../../src/core/ui/project/project_view.cpp
HEADERS += ../../src/core/management_layer/content/project/project_models_facade.h \
    ../../src/core/ui/project/project_view.h
