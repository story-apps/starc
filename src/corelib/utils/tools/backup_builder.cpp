#include "backup_builder.h"

#include <QDate>
#include <QDir>
#include <QString>

#include <set>


void BackupBuilder::save(const QString& _filePath, const QString& _backupDir,
                         const QString& _newName, int _maximumBackups)
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
    // резервные копии создаются по принципу _maximumBackups последних копий
    //

    QFileInfo fileInfo(_filePath);
    const QString backupBaseName = _newName.isEmpty() ? fileInfo.completeBaseName() : _newName;
    auto backupFileNameFor = [backupPath, backupBaseName, fileInfo](const QDateTime& _dateTime) {
        return QString("%1%2_%3.%4")
            .arg(backupPath, backupBaseName, _dateTime.toString("yyyy_MM_dd_hh_mm_ss"),
                 fileInfo.completeSuffix());
    };

    //
    // Создаём копию
    //
    const auto rightNow = QDateTime::currentDateTime();
    const QString tmpBackupFileName
        = QString("%1%2.tmp.%3").arg(backupPath, backupBaseName, fileInfo.completeSuffix());
    //
    // ... копируем файл во временную резервную копию
    //
    if (QFile::copy(_filePath, tmpBackupFileName)) {
        //
        // ... если скопировать удалось, переименовываем временную копию
        //
        const QString backupFileName = backupFileNameFor(rightNow);
        QFile::remove(backupFileName);
        QFile::rename(tmpBackupFileName, backupFileName);
    }

    //
    // Формируем список имеющихся резервных копий
    //
    const auto nameFilter = QString("%1_*.%2").arg(backupBaseName, fileInfo.completeSuffix());
    QVector<QString> backups;
    const auto files = QDir(_backupDir).entryInfoList({ nameFilter }, QDir::Files);
    for (const auto& file : files) {
        backups.append(file.absoluteFilePath());
    }
    //
    // ... помещаем сверху актуальные, а внизу старые
    //
    std::sort(backups.begin(), backups.end(), std::greater<QString>());

    //
    // Удаляем старые
    //
    while (backups.size() > _maximumBackups) {
        const auto backupToRemove = backups.takeLast();
        QFile::remove(backupToRemove);
    }
}
