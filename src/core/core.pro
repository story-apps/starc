#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:23:46
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin c++1z
QT += widgets sql xml

TARGET = $$qtLibraryTarget(coreplugin)

include(../cloud/cloud.pri)

DEFINES += CORE_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build/plugins

INCLUDEPATH += ..

SOURCES += \
    business_layer/import/abstract_importer.cpp \
    data_layer/database.cpp \
    data_layer/mapper/mapper_facade.cpp \
    data_layer/mapper/settings_mapper.cpp \
    data_layer/storage/settings_storage.cpp \
    data_layer/storage/storage_facade.cpp \
    domain/domain_object.cpp \
    domain/identifier.cpp \
    domain/item_object.cpp \
    domain/items_builder.cpp \
    management_layer/application_manager.cpp \
    management_layer/content/account/account_manager.cpp \
    management_layer/content/onboarding/onboarding_manager.cpp \
    management_layer/content/projects/project.cpp \
    management_layer/content/projects/projects_manager.cpp \
    management_layer/content/settings/settings_manager.cpp \
    ui/account/account_bar.cpp \
    ui/account/account_navigator.cpp \
    ui/account/account_tool_bar.cpp \
    ui/account/account_view.cpp \
    ui/account/avatar.cpp \
    ui/account/change_password_dialog.cpp \
    ui/account/login_dialog.cpp \
    ui/account/renew_subscription_dialog.cpp \
    ui/account/upgrade_to_pro_dialog.cpp \
    ui/application_style.cpp \
    ui/application_view.cpp \
    ui/design_system/design_system.cpp \
    ui/menu_view.cpp \
    ui/onboarding/onboarding_navigator.cpp \
    ui/onboarding/onboarding_tool_bar.cpp \
    ui/onboarding/onboarding_view.cpp \
    ui/projects/create_project_dialog.cpp \
    ui/projects/projects_cards.cpp \
    ui/projects/projects_navigator.cpp \
    ui/projects/projects_tool_bar.cpp \
    ui/projects/projects_view.cpp \
    ui/settings/language_dialog.cpp \
    ui/settings/settings_navigator.cpp \
    ui/settings/settings_tool_bar.cpp \
    ui/settings/settings_view.cpp \
    ui/settings/theme_dialog.cpp \
    ui/widgets/app_bar/app_bar.cpp \
    ui/widgets/button/button.cpp \
    ui/widgets/card/card.cpp \
    ui/widgets/check_box/check_box.cpp \
    ui/widgets/circular_progress_bar/circular_progress_bar.cpp \
    ui/widgets/combo_box/combo_box.cpp \
    ui/widgets/dialog/abstract_dialog.cpp \
    ui/widgets/dialog/dialog.cpp \
    ui/widgets/dialog/dialog_content.cpp \
    ui/widgets/dialog/standard_dialog.cpp \
    ui/widgets/drawer/drawer.cpp \
    ui/widgets/floating_tool_bar/floating_tool_bar.cpp \
    ui/widgets/label/label.cpp \
    ui/widgets/label/link_label.cpp \
    ui/widgets/radio_button/radio_button.cpp \
    ui/widgets/radio_button/radio_button_group.cpp \
    ui/widgets/scroll_bar/scroll_bar.cpp \
    ui/widgets/shadow/shadow.cpp \
    ui/widgets/slider/slider.cpp \
    ui/widgets/splitter/splitter.cpp \
    ui/widgets/splitter/splitter_handle.cpp \
    ui/widgets/stack_widget/stack_widget.cpp \
    ui/widgets/stepper/stepper.cpp \
    ui/widgets/text_field/text_field.cpp \
    ui/widgets/toggle_button/toggle_button.cpp \
    ui/widgets/tree/tree.cpp \
    ui/widgets/tree/tree_delegate.cpp \
    ui/widgets/tree/tree_view.cpp \
    ui/widgets/widget/widget.cpp \
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
    utils/helpers/text_helper.cpp \
    utils/helpers/quotes_helper.cpp \
    utils/tools/debouncer.cpp \
    utils/tools/run_once.cpp

HEADERS += \
    business_layer/import/abstract_importer.h \
    core_global.h \
    custom_events.h \
    data_layer/database.h \
    data_layer/mapper/mapper_facade.h \
    data_layer/mapper/settings_mapper.h \
    data_layer/storage/settings_storage.h \
    data_layer/storage/storage_facade.h \
    domain/domain_object.h \
    domain/identifier.h \
    domain/item_object.h \
    domain/items_builder.h \
    management_layer/application_manager.h \
    management_layer/content/account/account_manager.h \
    management_layer/content/onboarding/onboarding_manager.h \
    management_layer/content/projects/project.h \
    management_layer/content/projects/projects_manager.h \
    management_layer/content/settings/settings_manager.h \
    ui/account/account_bar.h \
    ui/account/account_navigator.h \
    ui/account/account_tool_bar.h \
    ui/account/account_view.h \
    ui/account/avatar.h \
    ui/account/change_password_dialog.h \
    ui/account/login_dialog.h \
    ui/account/renew_subscription_dialog.h \
    ui/account/upgrade_to_pro_dialog.h \
    ui/application_style.h \
    ui/application_view.h \
    ui/design_system/design_system.h \
    ui/menu_view.h \
    ui/onboarding/onboarding_navigator.h \
    ui/onboarding/onboarding_tool_bar.h \
    ui/onboarding/onboarding_view.h \
    ui/projects/create_project_dialog.h \
    ui/projects/projects_cards.h \
    ui/projects/projects_navigator.h \
    ui/projects/projects_tool_bar.h \
    ui/projects/projects_view.h \
    ui/settings/language_dialog.h \
    ui/settings/settings_navigator.h \
    ui/settings/settings_tool_bar.h \
    ui/settings/settings_view.h \
    ui/settings/theme_dialog.h \
    ui/widgets/app_bar/app_bar.h \
    ui/widgets/button/button.h \
    ui/widgets/card/card.h \
    ui/widgets/check_box/check_box.h \
    ui/widgets/circular_progress_bar/circular_progress_bar.h \
    ui/widgets/combo_box/combo_box.h \
    ui/widgets/dialog/abstract_dialog.h \
    ui/widgets/dialog/dialog.h \
    ui/widgets/dialog/dialog_content.h \
    ui/widgets/dialog/standard_dialog.h \
    ui/widgets/drawer/drawer.h \
    ui/widgets/floating_tool_bar/floating_tool_bar.h \
    ui/widgets/label/label.h \
    ui/widgets/label/link_label.h \
    ui/widgets/radio_button/radio_button.h \
    ui/widgets/radio_button/radio_button_group.h \
    ui/widgets/scroll_bar/scroll_bar.h \
    ui/widgets/shadow/shadow.h \
    ui/widgets/slider/slider.h \
    ui/widgets/splitter/splitter.h \
    ui/widgets/splitter/splitter_handle.h \
    ui/widgets/stack_widget/stack_widget.h \
    ui/widgets/stepper/stepper.h \
    ui/widgets/text_field/text_field.h \
    ui/widgets/toggle_button/toggle_button.h \
    ui/widgets/tree/tree.h \
    ui/widgets/tree/tree_delegate.h \
    ui/widgets/tree/tree_view.h \
    ui/widgets/widget/widget.h \
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
    utils/helpers/text_helper.h \
    utils/helpers/quotes_helper.h \
    utils/tools/debouncer.h \
    utils/tools/run_once.h \
    utils/validators/email_validator.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

RESOURCES += \
    resources.qrc \
    translations.qrc
