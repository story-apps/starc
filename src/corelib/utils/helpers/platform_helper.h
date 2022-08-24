#pragma once

#include <corelib_global.h>

class QWidget;


/**
 * @brief Платформозависимые вещи инкапсулируются тут
 */
class CORE_LIBRARY_EXPORT PlatformHelper
{
public:
    /**
     * @brief Настроить вывод в консоль
     */
    static void initConsoleOutput();

    /**
     * @brief Настроить заголовок окна в зависимости от темы
     */
    static void setTitleBarTheme(QWidget* _window, bool _isLightTheme);

    /**
     * @brief Получить имя файла, которое можно сохранить в системе
     */
    static QString systemSavebleFileName(const QString& _fileName);

    /**
     * @brief Показать файл в папке
     * https://github.com/qt-creator/qt-creator/blob/master/src/plugins/coreplugin/fileutils.cpp#L67
     * https://github.com/qbittorrent/qBittorrent/blob/e42fa0e027fddde1d4518d2d90d06bf2a64269d8/src/gui/utils.cpp
     */
    static bool showInGraphicalShell(const QString& _path);
};
