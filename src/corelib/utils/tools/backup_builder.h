#pragma once

class QString;


/**
 * @brief Класс для организации создания резервных копий
 */
class BackupBuilder
{
public:
    /**
     * @brief Сохранить бэкап
     */
    static void save(const QString& _filePath, const QString& _backupDir);
    static void save(const QString& _filePath, const QString& _backupDir, const QString& _newName);
};
