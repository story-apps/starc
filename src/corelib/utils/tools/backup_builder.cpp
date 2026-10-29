#include "backup_builder.h"

#include <utils/helpers/platform_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDate>
#include <QDir>
#include <QRegularExpression>
#include <QString>
#include <QSysInfo>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <cstring>
#include <dirent.h>
#endif


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
    //
    // Ищем файлы которые соответствуют имени проекта, а также учитываем кейс с файлами, которые
    // были выгружены в iCloud для macOS, у таких файлов добавляется точка в начале имени и .icloud
    // в конце
    //
    const QRegularExpression backupNamePattern(
        QStringLiteral("\\A\\.?%1_[0-9]{4}(?:_[0-9]{2}){5}\\.%2(?:\\.icloud)?\\z")
            .arg(normalizedAndEscaped(backupBaseName),
                 normalizedAndEscaped(fileInfo.completeSuffix())));
    QVector<QString> backups;
    const auto files
        = QDir(_backupDir)
              .entryInfoList(QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const auto& file : files) {
        if (backupNamePattern.match(normalized(file.fileName())).hasMatch()) {
            backups.append(file.absoluteFilePath());
        }
    }
    const auto foundBackupsAmount = backups.size();
    //
    // TODO: временная диагностика
    //
    {
        const QFileInfo directoryInfo(_backupDir);
        backupResult.diagnostics.append(
            QStringLiteral("[BackupBuilder] directory=\"%1\", canonical=\"%2\", exists=%3, "
                           "isDir=%4, readable=%5, writable=%6, executable=%7, permissions=0x%8, "
                           "ownerId=%9, groupId=%10, Qt=%11, OS=\"%12\"")
                .arg(directoryInfo.absoluteFilePath(), directoryInfo.canonicalFilePath())
                .arg(directoryInfo.exists())
                .arg(directoryInfo.isDir())
                .arg(directoryInfo.isReadable())
                .arg(directoryInfo.isWritable())
                .arg(directoryInfo.isExecutable())
                .arg(static_cast<uint>(directoryInfo.permissions()), 0, 16)
                .arg(directoryInfo.ownerId())
                .arg(directoryInfo.groupId())
                .arg(QString::fromLatin1(qVersion()), QSysInfo::prettyProductName()));
        backupResult.diagnostics.append(
            QStringLiteral("[BackupBuilder] created=\"%1\", existsByPath=%2, Qt entries=%3, "
                           "matching=%4, limit=%5, pattern=\"%6\", valid=%7, patternError=\"%8\", "
                           "createdNameMatches=%9, baseNameUtf8Hex=%10, suffixUtf8Hex=%11")
                .arg(backupFileName)
                .arg(QFileInfo::exists(backupFileName))
                .arg(files.size())
                .arg(foundBackupsAmount)
                .arg(_maximumBackups)
                .arg(backupNamePattern.pattern())
                .arg(backupNamePattern.isValid())
                .arg(backupNamePattern.errorString())
                .arg(backupNamePattern.match(normalized(QFileInfo(backupFileName).fileName()))
                         .hasMatch())
                .arg(QString::fromLatin1(backupBaseName.toUtf8().toHex()),
                     QString::fromLatin1(fileInfo.completeSuffix().toUtf8().toHex())));
        const auto normalizedBackupPath = normalized(QFileInfo(backupFileName).absoluteFilePath());
        if (std::none_of(backups.cbegin(), backups.cend(), [&](const QString& _path) {
                return normalized(_path) == normalizedBackupPath;
            })) {
            // QFileInfo access flags do not prove that directory enumeration is allowed.
            // Qt does not expose the enumeration error, so also query the native API on Unix/macOS.
            const int sampleLimit = 20;
            for (int index = 0; index < qMin(sampleLimit, int(files.size())); ++index) {
                const auto& file = files.at(index);
                backupResult.diagnostics.append(
                    QStringLiteral(
                        "[BackupBuilder] Qt entry=\"%1\", utf8Hex=%2, normalizedUtf8Hex=%3, "
                        "isFile=%4, isSymLink=%5, matches=%6")
                        .arg(file.fileName(), QString::fromLatin1(file.fileName().toUtf8().toHex()),
                             QString::fromLatin1(normalized(file.fileName()).toUtf8().toHex()))
                        .arg(file.isFile())
                        .arg(file.isSymLink())
                        .arg(backupNamePattern.match(normalized(file.fileName())).hasMatch()));
            }
#ifdef Q_OS_UNIX
            const auto directoryPath = QFile::encodeName(QDir(_backupDir).absolutePath());
            DIR* directory = ::opendir(directoryPath.constData());
            const int openError = directory ? 0 : errno;
            backupResult.diagnostics.append(
                QStringLiteral("[BackupBuilder] native opendir: success=%1, errno=%2, error=\"%3\"")
                    .arg(directory != nullptr)
                    .arg(openError)
                    .arg(QString::fromLocal8Bit(std::strerror(openError))));
            if (directory) {
                int entries = 0;
                int readError = 0;
                for (;;) {
                    errno = 0;
                    const auto entry = ::readdir(directory);
                    if (!entry) {
                        readError = errno;
                        break;
                    }
                    if (std::strcmp(entry->d_name, ".") == 0
                        || std::strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }
                    if (entries++ < sampleLimit) {
                        backupResult.diagnostics.append(
                            QStringLiteral("[BackupBuilder] native entry=\"%1\", rawNameHex=%2")
                                .arg(QFile::decodeName(entry->d_name),
                                     QString::fromLatin1(QByteArray(entry->d_name).toHex())));
                    }
                }
                ::closedir(directory);
                backupResult.diagnostics.append(
                    QStringLiteral(
                        "[BackupBuilder] native readdir: entries=%1, errno=%2, error=\"%3\"; "
                        "Qt/native name samples limited to %4 each")
                        .arg(entries)
                        .arg(readError)
                        .arg(QString::fromLocal8Bit(std::strerror(readError)))
                        .arg(sampleLimit));
            }
#endif
            backupResult.success = false;
            backupResult.status
                = QStringLiteral(
                      "Backup created at \"%1\", but not found during cleanup: "
                      "directory=\"%2\", entries=%3, matching backups=%4, pattern=\"%5\", "
                      "pattern error=\"%6\"")
                      .arg(backupFileName, _backupDir)
                      .arg(files.size())
                      .arg(foundBackupsAmount)
                      .arg(backupNamePattern.pattern(), backupNamePattern.errorString());
            return backupResult;
        }
    }
    //
    // ... помещаем сверху актуальные, а внизу старые
    //
    std::sort(backups.begin(), backups.end(), [](const QString& _lhs, const QString& _rhs) {
        //
        // ... учитываем скрытые копии из iCloud
        //
        auto clearName = [](const QString& _path) {
            const auto fileName = QFileInfo(_path).fileName();
            const QLatin1String icloudSuffix(".icloud");
            if (fileName.startsWith('.') && fileName.endsWith(icloudSuffix)) {
                return fileName.mid(1, fileName.size() - 1 - icloudSuffix.size());
            }
            return fileName;
        };
        return clearName(_lhs) > clearName(_rhs);
    });

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

    backupResult.status = QString("Backup created: path=\"%1\", matching backups=%2, removed=%3")
                              .arg(backupFileName)
                              .arg(foundBackupsAmount)
                              .arg(removedBackupsAmount);
    return backupResult;
}
