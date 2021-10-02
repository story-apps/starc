#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:23:46
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin c++1z
QT += concurrent widgets multimedia network

TARGET = coreplugin

exists("../cloud/cloud.pri") {
    include(../cloud/cloud.pri)
}

DEFINES += CORE_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

mac {
    DESTDIR = ../_build/starcapp.app/Contents/PlugIns
    CORELIBDIR = ../_build/starcapp.app/Contents/Frameworks
} else {
    DESTDIR = ../_build/plugins
    CORELIBDIR = ../_build
}
LIBSDIR = ../_build/libs

INCLUDEPATH += ..

#
# Подключаем библиотеку corelib
#
LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../corelib
DEPENDPATH += $$PWD/../corelib
#

#
# Подключаем библиотеку Webloader
#
LIBS += -L$$LIBSDIR/ -lwebloader
INCLUDEPATH += $$PWD/../3rd_party/webloader/src
DEPENDPATH += $$PWD/../3rd_party/webloader
#

SOURCES += \
    management_layer/application_manager.cpp \
    management_layer/content/account/account_manager.cpp \
    management_layer/content/export/export_manager.cpp \
    management_layer/content/import/import_manager.cpp \
    management_layer/content/onboarding/onboarding_manager.cpp \
    management_layer/content/project/project_manager.cpp \
    management_layer/content/project/project_models_facade.cpp \
    management_layer/content/project/project_plugins_builder.cpp \
    management_layer/content/projects/project.cpp \
    management_layer/content/projects/projects_manager.cpp \
    management_layer/content/settings/screenplay_template_manager.cpp \
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
    ui/export/comic_book_export_dialog.cpp \
    ui/export/screenplay_export_dialog.cpp \
    ui/import/import_dialog.cpp \
    ui/menu_view.cpp \
    ui/onboarding/onboarding_navigator.cpp \
    ui/onboarding/onboarding_tool_bar.cpp \
    ui/onboarding/onboarding_view.cpp \
    ui/project/create_document_dialog.cpp \
    ui/project/project_navigator.cpp \
    ui/project/project_tool_bar.cpp \
    ui/project/project_tree_delegate.cpp \
    ui/project/project_view.cpp \
    ui/projects/create_project_dialog.cpp \
    ui/projects/projects_cards.cpp \
    ui/projects/projects_navigator.cpp \
    ui/projects/projects_tool_bar.cpp \
    ui/projects/projects_view.cpp \
    ui/settings/language_dialog.cpp \
    ui/settings/screenplay_template/screenplay_template_navigator.cpp \
    ui/settings/screenplay_template/screenplay_template_page_view.cpp \
    ui/settings/screenplay_template/screenplay_template_paragraphs_view.cpp \
    ui/settings/screenplay_template/screenplay_template_tool_bar.cpp \
    ui/settings/screenplay_template/screenplay_template_view_tool_bar.cpp \
    ui/settings/settings_navigator.cpp \
    ui/settings/settings_tool_bar.cpp \
    ui/settings/settings_view.cpp \
    ui/settings/theme_dialog.cpp \
    ui/settings/widgets/page_layout.cpp

HEADERS += \
    core_global.h \
    management_layer/application_manager.h \
    management_layer/content/account/account_manager.h \
    management_layer/content/export/export_manager.h \
    management_layer/content/import/import_manager.h \
    management_layer/content/onboarding/onboarding_manager.h \
    management_layer/content/project/project_manager.h \
    management_layer/content/project/project_models_facade.h \
    management_layer/content/project/project_plugins_builder.h \
    management_layer/content/projects/project.h \
    management_layer/content/projects/projects_manager.h \
    management_layer/content/settings/screenplay_template_manager.h \
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
    ui/export/comic_book_export_dialog.h \
    ui/export/screenplay_export_dialog.h \
    ui/import/import_dialog.h \
    ui/menu_view.h \
    ui/onboarding/onboarding_navigator.h \
    ui/onboarding/onboarding_tool_bar.h \
    ui/onboarding/onboarding_view.h \
    ui/project/create_document_dialog.h \
    ui/project/project_navigator.h \
    ui/project/project_tool_bar.h \
    ui/project/project_tree_delegate.h \
    ui/project/project_view.h \
    ui/projects/create_project_dialog.h \
    ui/projects/projects_cards.h \
    ui/projects/projects_navigator.h \
    ui/projects/projects_tool_bar.h \
    ui/projects/projects_view.h \
    ui/settings/language_dialog.h \
    ui/settings/screenplay_template/screenplay_template_navigator.h \
    ui/settings/screenplay_template/screenplay_template_page_view.h \
    ui/settings/screenplay_template/screenplay_template_paragraphs_view.h \
    ui/settings/screenplay_template/screenplay_template_tool_bar.h \
    ui/settings/screenplay_template/screenplay_template_view_tool_bar.h \
    ui/settings/settings_navigator.h \
    ui/settings/settings_tool_bar.h \
    ui/settings/settings_view.h \
    ui/settings/theme_dialog.h \
    ui/settings/widgets/page_layout.h

RESOURCES += \
    resources.qrc \
    translations.qrc

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
