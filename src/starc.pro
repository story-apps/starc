TEMPLATE = subdirs ordered

SUBDIRS += \
    3rd_party \
    corelib \
    core/management_layer/plugins \
    core \
    app

CONFIG += ordered

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/management_layer/i_document_manager.h

TRANSLATIONS += \
    core/translations/_en_source.ts \
    core/translations/for_use_starc_translation_az.ts \
    core/translations/for_use_starc_translation_de.ts \
    core/translations/for_use_starc_translation_es.ts \
    core/translations/for_use_starc_translation_he.ts \
    core/translations/for_use_starc_translation_hi.ts \
    core/translations/for_use_starc_translation_hu.ts \
    core/translations/for_use_starc_translation_it.ts \
    core/translations/for_use_starc_translation_ru.ts \
    core/translations/for_use_starc_translation_tr.ts \
    core/translations/for_use_starc_translation_sl.ts \
    core/translations/for_use_starc_translation_uk.ts

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
