TARGET   = hunspell
TEMPLATE = lib

#
# Build configuration
#
CONFIG += qt thread warn_on staticlib
QT -= core gui

#
# Конфигурируем расположение файлов сборки
#
DESTDIR = ../../_build/libs
#

#
# Настройка статической линковки под Windows
#
win32 {
    DEFINES += HUNSPELL_STATIC WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS _MBCS
}

#
# Для unix запускаем конфигурирование для создания config.h, но собираем сами
#
unix {
    system(./configure --quiet)
}
#
# Для Win32 используем специализированную конфигурацию, поставляемую с hunspell
#
else: win32 {
    INCLUDEPATH += src/win_api

    HEADERS += \
        src/win_api/config.h \
        src/win_api/hunspelldll.h

    SOURCES += \
        src/win_api/hunspelldll.cxx
}

#
# Пути поиска заголовочных файлов
#
INCLUDEPATH += src/hunspell

#
# Заголовочные файлы
#
HEADERS += \
    src/hunspell/affentry.hxx \
    src/hunspell/htypes.hxx \
    src/hunspell/affixmgr.hxx \
    src/hunspell/csutil.hxx \
    src/hunspell/hunspell.hxx \
    src/hunspell/atypes.hxx \
    src/hunspell/dictmgr.hxx \
    src/hunspell/hunspell.h \
    src/hunspell/suggestmgr.hxx \
    src/hunspell/baseaffix.hxx \
    src/hunspell/hashmgr.hxx \
    src/hunspell/langnum.hxx \
    src/hunspell/phonet.hxx \
    src/hunspell/filemgr.hxx \
    src/hunspell/hunzip.hxx \
    src/hunspell/w_char.hxx \
    src/hunspell/replist.hxx \
    src/hunspell/hunvisapi.h

#
# Исходные тексты
#
SOURCES += \
    src/hunspell/affentry.cxx \
    src/hunspell/affixmgr.cxx \
    src/hunspell/csutil.cxx \
    src/hunspell/dictmgr.cxx \
    src/hunspell/hashmgr.cxx \
    src/hunspell/hunspell.cxx \
    src/hunspell/suggestmgr.cxx \
    src/hunspell/license.myspell \
    src/hunspell/license.hunspell \
    src/hunspell/phonet.cxx \
    src/hunspell/filemgr.cxx \
    src/hunspell/hunzip.cxx \
    src/hunspell/replist.cxx \
    src/hunspell/utf_info.cxx
