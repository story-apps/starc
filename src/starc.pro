TEMPLATE = subdirs

SUBDIRS += \
    app \
    core \
    core/management_layer/plugins \
    corelib \
    3rd_party

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/management_layer/i_document_manager.h

TRANSLATIONS += \
    core/translations/en_EN.ts \
    core/translations/ru_RU.ts

CONFIG += qt

QT += widgets
