TEMPLATE = lib

CONFIG += plugin c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += widgets

TARGET = screenplaybreakdownstructureplugin

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
    business_layer/screenplay_breakdown_structure_characters_model.h \
    business_layer/screenplay_breakdown_structure_locations_model.h \
    business_layer/screenplay_breakdown_structure_model_item.h \
    business_layer/screenplay_breakdown_structure_model_proxy.h \
    business_layer/screenplay_breakdown_structure_scenes_model.h \
    screenplay_breakdown_structure_manager.h \
    ui/counters_info_widget.h \
    ui/screenplay_breakdown_structure_scene_delegate.h \
    ui/screenplay_breakdown_structure_tag_delegate.h \
    ui/screenplay_breakdown_structure_view.h

SOURCES += \
    business_layer/screenplay_breakdown_structure_characters_model.cpp \
    business_layer/screenplay_breakdown_structure_locations_model.cpp \
    business_layer/screenplay_breakdown_structure_model_item.cpp \
    business_layer/screenplay_breakdown_structure_model_proxy.cpp \
    business_layer/screenplay_breakdown_structure_scenes_model.cpp \
    screenplay_breakdown_structure_manager.cpp \
    ui/counters_info_widget.cpp \
    ui/screenplay_breakdown_structure_scene_delegate.cpp \
    ui/screenplay_breakdown_structure_tag_delegate.cpp \
    ui/screenplay_breakdown_structure_view.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
