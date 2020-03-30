#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Класс для организации создания резервных копий
 */
class CORE_LIBRARY_EXPORT BackupBuilder
{
public:
    /**
     * @brief Сохранить бэкап
     */
    static void save(const QString& _filePath, const QString& _backupDir);
    static void save(const QString& _filePath, const QString& _backupDir, const QString& _newName);
};
