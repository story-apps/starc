#pragma once

#include <QString>

#include <corelib_global.h>


/**
 * @brief Класс для организации создания резервных копий
 */
namespace BackupBuilder {

/**
 * @brief Результат выполнения операции по созданию бекапа
 */
struct CORE_LIBRARY_EXPORT BackupResult {
    bool success = false;
    QString error;
};


/**
 * @brief Сохранить бэкап
 */
CORE_LIBRARY_EXPORT extern BackupResult save(const QString& _filePath, const QString& _backupDir,
                                             const QString& _newName, int _maximumBackups);

} // namespace BackupBuilder
