#include "backup_builder.h"

#include <QDate>
#include <QDir>
#include <QString>

#include <set>


void BackupBuilder::save(const QString& _filePath, const QString& _backupDir,
                         const QString& _newName)
{
    //
    // Создаём папку для хранения резервных копий, если такой ещё нет
    //
    QDir::root().mkpath(_backupDir);

    //
    // Сформируем путь к резервной копии
    //
    QString backupPath = _backupDir;
    if (!backupPath.endsWith(QDir::separator())) {
        backupPath.append(QDir::separator());
    }

    //
    // резервные копии создаются по принципу
    // 1 копия двухнедельной давности
    // 1 копия недельной давности
    // 1 копия за позавчера
    // 1 копия за вчера
    // 1 копия за сегодня
    //

    QFileInfo fileInfo(_filePath);
    const QString backupBaseName = _newName.isEmpty() ? fileInfo.completeBaseName() : _newName;
    auto backupFileNameFor = [backupPath, backupBaseName, fileInfo](const QDate& _date) {
        return QString("%1%2_%3.%4")
            .arg(backupPath, backupBaseName, _date.toString("yyyy_MM_dd"),
                 fileInfo.completeSuffix());
    };

    //
    // Создаём сегодняшнюю копию
    //
    const auto today = QDate::currentDate();
    const QString tmpBackupFileName
        = QString("%1%2.tmp.%3").arg(backupPath, backupBaseName, fileInfo.completeSuffix());
    //
    // ... копируем файл во временную резервную копию
    //
    if (QFile::copy(_filePath, tmpBackupFileName)) {
        //
        // ... если скопировать удалось, переименовываем временную копию
        //
        const QString backupFileName = backupFileNameFor(today);
        QFile::remove(backupFileName);
        QFile::rename(tmpBackupFileName, backupFileName);
    }

    //
    // Формируем список имеющихся резервных копий
    //
    const auto nameFilter = QString("%1_*.%2").arg(backupBaseName, fileInfo.completeSuffix());
    std::set<QString> backups;
    const auto files = QDir(_backupDir).entryInfoList({ nameFilter }, QDir::Files);
    for (const auto& file : files) {
        backups.insert(file.absoluteFilePath());
    }

    //
    // Если ещё не накопилось достаточно бэкапов, не будем удалять старые
    //
    const int minimumBackupsSize = 8;
    if (backups.size() <= minimumBackupsSize) {
        return;
    }

    //
    // Удаляем потерявшие актуальность резервные копии
    //
    auto backupsToRemove = backups;
    //
    // Оставляем бэкапы за последнюю неделю
    //
    for (int dayOffset = 0; dayOffset < 7; ++dayOffset) {
        backupsToRemove.erase(backupFileNameFor(today.addDays(-dayOffset)));
    }

    //
    // Определим необходимое кол-во элементов к удалению
    //
    auto itemsToRemove = backups.size() - minimumBackupsSize;

    //
    // Удаляем в индервале [14 дней назад; 7 дней назад] самые свежие
    //
    {
        std::set<QString> lastWeekBackups;
        std::copy(backupsToRemove.lower_bound(backupFileNameFor(today.addDays(-13))),
                  backupsToRemove.end(), std::inserter(lastWeekBackups, lastWeekBackups.begin()));
        while (itemsToRemove > 0 && lastWeekBackups.size() > 1) {
            const auto backup = *lastWeekBackups.rbegin();
            lastWeekBackups.erase(backup);
            QFile::remove(backup);
            --itemsToRemove;
        }
    }

    //
    // Удаляем в индервале [самый старый бэкап; 14 дней назад] самые древние бэкапы
    //
    {
        std::set<QString> oldBackups;
        std::copy(backupsToRemove.begin(),
                  backupsToRemove.lower_bound(backupFileNameFor(today.addDays(-13))),
                  std::inserter(oldBackups, oldBackups.begin()));
        while (itemsToRemove > 0 && oldBackups.size() > 1) {
            const auto backup = *oldBackups.begin();
            oldBackups.erase(backup);
            QFile::remove(backup);
            --itemsToRemove;
        }
    }
}
