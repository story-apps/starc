#include "backup_builder.h"

#include <utils/helpers/platform_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDate>
#include <QDir>
#include <QRegularExpression>
#include <QString>


BackupBuilder::BackupResult BackupBuilder::save(const QString& _filePath, const QString& _backupDir,
                                                const QString& _newName, int _maximumBackups)
{
    BackupResult backupResult;
    backupResult.success = true;

    //
    // Создаём папку для хранения резервных копий, если такой ещё нет
    //
    if (QDir::root().mkpath(_backupDir) == false) {
        backupResult.success = false;
        backupResult.status = QObject::tr("Can't create backups folder \"%1\". Please check "
                                          "permissions for backups filder, or create it manually.",
                                          "BackupBuilder")
                                  .arg(_backupDir);
        return backupResult;
    }

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
    if (QFile::copy(_filePath, tmpBackupFileName) == false) {
        backupResult.success = false;
        backupResult.status
            = QObject::tr("Can't copy your project \"%1\" to temporary backup \"%2\". Please check "
                          "permissions and provide ability for writing to backups folder.",
                          "BackupBuilder")
                  .arg(_filePath, tmpBackupFileName);
        return backupResult;
    }
    //
    // ... если скопировать удалось, переименовываем временную копию
    //
    const QString backupFileName = backupFileNameFor(rightNow);
    if (QFile::exists(backupFileName) && QFile::remove(backupFileName) == false) {
        QFile::remove(tmpBackupFileName);
        backupResult.success = false;
        backupResult.status
            = QObject::tr("Can't replace existing backup \"%1\". Please check permissions and "
                          "provide ability for writing to backups folder.",
                          "BackupBuilder")
                  .arg(backupFileName);
        return backupResult;
    }
    if (QFile::rename(tmpBackupFileName, backupFileName) == false) {
        QFile::remove(tmpBackupFileName);
        backupResult.success = false;
        backupResult.status
            = QObject::tr("Can't rename temporary backup \"%1\" to \"%2\". Please check "
                          "permissions and provide ability for writing to backups folder.",
                          "BackupBuilder")
                  .arg(tmpBackupFileName, backupFileName);
        return backupResult;
    }

    //
    // Формируем список имеющихся резервных копий
    //
    // Используем нормализацию для имён файлов, чтобы не зависеть от причуд операционки
    //
    auto normalized = [](const QString& _text) { return PlatformHelper::normalizePath(_text); };
    auto normalizedAndEscaped = [&normalized](const QString& _text) {
        return TextHelper::toRxEscaped(normalized(_text));
    };
    const QRegularExpression backupNamePattern(
        QStringLiteral("^%1_(.*)[.]%2$")
            .arg(normalizedAndEscaped(backupBaseName),
                 normalizedAndEscaped(fileInfo.completeSuffix())));
    QVector<QString> backups;
    const auto files = QDir(_backupDir).entryInfoList(QDir::Files);
    for (const auto& file : files) {
        if (backupNamePattern.match(normalized(file.fileName())).hasMatch()) {
            backups.append(file.absoluteFilePath());
        }
    }
    const auto foundBackupsAmount = backups.size();
    //
    // ... помещаем сверху актуальные, а внизу старые
    //
    std::sort(backups.begin(), backups.end(), std::greater<QString>());

    //
    // Удаляем старые
    //
    int removedBackupsAmount = 0;
    while (backups.size() > _maximumBackups) {
        const auto backupToRemove = backups.takeLast();
        if (!QFile::remove(backupToRemove)) {
            backupResult.success = false;
            backupResult.status
                = QObject::tr("Can't remove old backup \"%1\". Found %2 backups while the limit "
                              "is %3. Please check permissions for the backups folder.",
                              "BackupBuilder")
                      .arg(backupToRemove)
                      .arg(foundBackupsAmount)
                      .arg(_maximumBackups);
            return backupResult;
        }
        ++removedBackupsAmount;
    }

    backupResult.status
        = QObject::tr("Backup created: path=\"%1\", matching backups=%2, removed=%3",
                      "BackupBuilder")
              .arg(backupFileName)
              .arg(foundBackupsAmount)
              .arg(removedBackupsAmount);
    return backupResult;
}
