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

#
# Настраиваем вывод в консоль в Windows в нормальной кодировке
#
win32 {
    QMAKE_EXTRA_TARGETS += before_build makefilehook

    makefilehook.target = $(MAKEFILE)
    makefilehook.depends = .beforebuild

    PRE_TARGETDEPS += .beforebuild

    before_build.target = .beforebuild
    before_build.depends = FORCE
    before_build.commands = chcp 1251
}
