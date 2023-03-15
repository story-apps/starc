#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Класс для организации создания резервных копий
 */
namespace BackupBuilder {

/**
 * @brief Сохранить бэкап
 */
CORE_LIBRARY_EXPORT extern void save(const QString& _filePath, const QString& _backupDir,
                                     const QString& _newName, int _maximumBackups);

} // namespace BackupBuilder
