TEMPLATE = subdirs

SUBDIRS += \
    app \
    core \
    corelib \
    core/ui/project/views/project_information

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/ui/i_document_navigator.h \
    interfaces/ui/i_document_plugin.h

TRANSLATIONS += \
    core/translations/en_EN.ts \
    core/translations/ru_RU.ts
