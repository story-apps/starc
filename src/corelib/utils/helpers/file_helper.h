#pragma once

class QString;

/**
 * @brief Вспомогательные функции для работы с файлами
 */
class FileHelper
{
public:
    /**
     * @brief Получить имя файла, которое можно сохранить в системе
     */
    static QString systemSavebleFileName(const QString& _fileName);
};
