#pragma once

#include <corelib_global.h>

class QString;

/**
 * @brief Вспомогательные функции для работы с файлами
 */
class CORE_LIBRARY_EXPORT FileHelper
{
public:
    /**
     * @brief Получить имя файла, которое можно сохранить в системе
     */
    static QString systemSavebleFileName(const QString& _fileName);

    /**
     * @brief Показать файл в папке
     * https://github.com/qt-creator/qt-creator/blob/master/src/plugins/coreplugin/fileutils.cpp#L67
     * https://github.com/qbittorrent/qBittorrent/blob/e42fa0e027fddde1d4518d2d90d06bf2a64269d8/src/gui/utils.cpp
     */
    static bool showInGraphicalShell(const QString& _pathIn);
};
