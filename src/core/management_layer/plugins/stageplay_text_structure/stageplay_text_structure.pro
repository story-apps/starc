TEMPLATE = lib

CONFIG += plugin c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += widgets

TARGET = stageplaytextstructureplugin

DEFINES += MANAGER_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

mac {
    DESTDIR = ../../../../_build/starcapp.app/Contents/PlugIns
    CORELIBDIR = ../../../../_build/starcapp.app/Contents/Frameworks
} else {
    DESTDIR = ../../../../_build/plugins
    CORELIBDIR = ../../../../_build
}

INCLUDEPATH += $$PWD/../../../..

#
# Подключаем библиотеку corelib
#

LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../../../../corelib
DEPENDPATH += $$PWD/../../../../corelib

#

HEADERS += \
    business_layer/stageplay_text_structure_model.h \
    stageplay_text_structure_manager.h \
    ui/stageplay_text_structure_delegate.h \
    ui/stageplay_text_structure_view.h

SOURCES += \
    business_layer/stageplay_text_structure_model.cpp \
    stageplay_text_structure_manager.cpp \
    ui/stageplay_text_structure_delegate.cpp \
    ui/stageplay_text_structure_view.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
