TEMPLATE = subdirs ordered

SUBDIRS += \
    app \
    3rd_party \
    corelib \
    core/management_layer/plugins \
    core \
    testapp

CONFIG += ordered

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/management_layer/i_document_manager.h

TRANSLATIONS += \
    core/translations/_en_source.ts \
    core/translations/translation_az.ts \
    core/translations/translation_be.ts \
    core/translations/translation_de.ts \
    core/translations/translation_en.ts \
    core/translations/translation_es.ts \
    core/translations/translation_fa.ts \
    core/translations/translation_fr.ts \
    core/translations/translation_gl_ES.ts \
    core/translations/translation_he.ts \
    core/translations/translation_hi.ts \
    core/translations/translation_hu.ts \
    core/translations/translation_id.ts \
    core/translations/translation_it.ts \
    core/translations/translation_pl.ts \
    core/translations/translation_pt_BR.ts \
    core/translations/translation_ro_RO.ts \
    core/translations/translation_ru.ts \
    core/translations/translation_sl.ts \
    core/translations/translation_tr.ts \
    core/translations/translation_uk.ts

#
# Настраиваем вывод в консоль в Windows в нормальной кодировке
#
win32-msvc* {
    QMAKE_EXTRA_TARGETS += before_build makefilehook

    makefilehook.target = $(MAKEFILE)
    makefilehook.depends = .beforebuild

    PRE_TARGETDEPS += .beforebuild

    before_build.target = .beforebuild
    before_build.depends = FORCE
    before_build.commands = chcp 1251
}
