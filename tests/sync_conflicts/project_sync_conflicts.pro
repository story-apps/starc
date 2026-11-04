include(sync_conflicts.pro)
TARGET = tst_project_sync_conflicts
QT += sql
INCLUDEPATH += $$PWD/../../src $$PWD/../../src/core
LIBS += -L$$STARC_BUILD/plugins -lcoreplugin
QMAKE_RPATHDIR += $$STARC_BUILD/plugins
SOURCES = tst_project_sync_conflicts.cpp
