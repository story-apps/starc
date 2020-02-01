TEMPLATE = subdirs

SUBDIRS += \
    app \
    core \
    core/management_layer/plugins/project_information \
    core/management_layer/plugins/screenplay_information \
    core/management_layer/plugins/screenplay_parameters \
    core/management_layer/plugins/text \
    corelib

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/management_layer/i_document_manager.h

TRANSLATIONS += \
    core/translations/en_EN.ts \
    core/translations/ru_RU.ts
