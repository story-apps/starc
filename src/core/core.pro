#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:23:46
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin c++11
QT += widgets sql

TARGET = $$qtLibraryTarget(coreplugin)

DEFINES += CORE_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build/plugins

INCLUDEPATH += ..

SOURCES += \
        management_layer/application_manager.cpp \
    data_layer/database.cpp \
    domain/domain_object.cpp \
    domain/identifier.cpp \
    management_layer/content/onboarding/onboarding_manager.cpp \
    management_layer/content/projects/projects_manager.cpp \
    ui/application_view.cpp \
    ui/widgets/stack_widget/stack_widget.cpp \
    ui/widgets/widget/widget.cpp \
    ui/design_system/design_system.cpp \
    utils/helpers/color_helper.cpp \
    utils/helpers/image_helper.cpp \
    ui/widgets/splitter/splitter.cpp \
    ui/widgets/splitter/splitter_handle.cpp \
    ui/onboarding/onboarding_navigator.cpp \
    ui/onboarding/onboarding_tool_bar.cpp \
    ui/onboarding/onboarding_view.cpp \
    ui/widgets/stepper/stepper.cpp \
    ui/widgets/button/button.cpp \
    ui/widgets/label/label.cpp \
    ui/widgets/radio_button/radio_button.cpp \
    ui/widgets/radio_button/radio_button_group.cpp \
    ui/widgets/label/link_label.cpp \
    ui/widgets/slider/slider.cpp \
    utils/helpers/text_helper.cpp \
    data_layer/mapper/mapper_facade.cpp \
    data_layer/mapper/settings_mapper.cpp \
    data_layer/storage/storage_facade.cpp \
    data_layer/storage/settings_storage.cpp \
    ui/projects/projects_navigator.cpp \
    ui/projects/projects_tool_bar.cpp \
    ui/projects/projects_view.cpp \
    ui/widgets/app_bar/app_bar.cpp \
    utils/helpers/quotes_helper.cpp

HEADERS += \
        core_global.h \
        management_layer/application_manager.h \
        data_layer/database.h \
        domain/domain_object.h \
        domain/identifier.h \
        management_layer/content/onboarding/onboarding_manager.h \
        management_layer/content/projects/projects_manager.h \
        ui/application_view.h \
    ui/widgets/stack_widget/stack_widget.h \
    ui/widgets/widget/widget.h \
    ui/design_system/design_system.h \
    utils/helpers/color_helper.h \
    utils/helpers/image_helper.h \
    ui/widgets/splitter/splitter.h \
    ui/widgets/splitter/splitter_handle.h \
    ui/onboarding/onboarding_navigator.h \
    ui/onboarding/onboarding_tool_bar.h \
    ui/onboarding/onboarding_view.h \
    ui/widgets/stepper/stepper.h \
    custom_events.h \
    ui/widgets/button/button.h \
    ui/widgets/label/label.h \
    ui/widgets/radio_button/radio_button.h \
    ui/widgets/radio_button/radio_button_group.h \
    ui/widgets/label/link_label.h \
    ui/widgets/slider/slider.h \
    utils/helpers/text_helper.h \
    data_layer/mapper/mapper_facade.h \
    data_layer/mapper/settings_mapper.h \
    data_layer/storage/storage_facade.h \
    data_layer/storage/settings_storage.h \
    ui/projects/projects_navigator.h \
    ui/projects/projects_tool_bar.h \
    ui/projects/projects_view.h \
    ui/widgets/app_bar/app_bar.h \
    utils/helpers/quotes_helper.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

RESOURCES += \
    resources.qrc \
    translations.qrc
