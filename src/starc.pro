TEMPLATE = subdirs

SUBDIRS += \
    app \
    core

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/ui/i_document_editor.h \
    interfaces/ui/i_document_navigator.h

TRANSLATIONS += \
    core/translations/en_EN.ts \
    core/translations/ru_RU.ts
