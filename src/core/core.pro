#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:23:46
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin c++1z
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
    ui/menu_view.cpp \
    ui/widgets/drawer/drawer.cpp \
    ui/widgets/stack_widget/stack_widget.cpp \
    ui/widgets/widget/widget.cpp \
    ui/design_system/design_system.cpp \
    utils/3rd_party/WAF/Animation/Animation.cpp \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillAnimator.cpp \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillDecorator.cpp \
    utils/3rd_party/WAF/Animation/Expand/ExpandAnimator.cpp \
    utils/3rd_party/WAF/Animation/Expand/ExpandDecorator.cpp \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideAnimator.cpp \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideDecorator.cpp \
    utils/3rd_party/WAF/Animation/Slide/SlideAnimator.cpp \
    utils/3rd_party/WAF/Animation/Slide/SlideForegroundDecorator.cpp \
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
    utils/helpers/quotes_helper.cpp \
    ui/widgets/floating_tool_bar/floating_tool_bar.cpp \
    ui/widgets/shadow/shadow.cpp \
    ui/projects/projects_cards.cpp \
    domain/project.cpp \
    ui/widgets/dialog/abstract_dialog.cpp \
    ui/projects/create_project_dialog.cpp \
    ui/widgets/dialog/dialog_content.cpp \
    ui/widgets/text_field/text_field.cpp \
    ui/widgets/toggle_button/toggle_button.cpp \
    ui/widgets/scroll_bar/scroll_bar.cpp \
    ui/application_style.cpp

HEADERS += \
        core_global.h \
        management_layer/application_manager.h \
        data_layer/database.h \
        domain/domain_object.h \
        domain/identifier.h \
        management_layer/content/onboarding/onboarding_manager.h \
        management_layer/content/projects/projects_manager.h \
        ui/application_view.h \
    ui/menu_view.h \
    ui/widgets/drawer/drawer.h \
    ui/widgets/stack_widget/stack_widget.h \
    ui/widgets/widget/widget.h \
    ui/design_system/design_system.h \
    utils/3rd_party/WAF/AbstractAnimator.h \
    utils/3rd_party/WAF/Animation/Animation.h \
    utils/3rd_party/WAF/Animation/AnimationPrivate.h \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillAnimator.h \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillDecorator.h \
    utils/3rd_party/WAF/Animation/Expand/ExpandAnimator.h \
    utils/3rd_party/WAF/Animation/Expand/ExpandDecorator.h \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideAnimator.h \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideDecorator.h \
    utils/3rd_party/WAF/Animation/Slide/SlideAnimator.h \
    utils/3rd_party/WAF/Animation/Slide/SlideForegroundDecorator.h \
    utils/3rd_party/WAF/WAF.h \
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
    utils/helpers/quotes_helper.h \
    ui/widgets/floating_tool_bar/floating_tool_bar.h \
    ui/widgets/shadow/shadow.h \
    ui/projects/projects_cards.h \
    domain/project.h \
    ui/widgets/dialog/abstract_dialog.h \
    ui/projects/create_project_dialog.h \
    ui/widgets/dialog/dialog_content.h \
    ui/widgets/text_field/text_field.h \
    ui/widgets/toggle_button/toggle_button.h \
    ui/widgets/scroll_bar/scroll_bar.h \
    ui/application_style.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

RESOURCES += \
    resources.qrc \
    translations.qrc
