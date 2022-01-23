#include "logging.h"

#include <QDateTime>
#include <QDir>
#include <QVariant>

#include <iostream>


Log::Level Log::m_logLevel = Log::Level::Warning;
QFile Log::m_logFile;

void Log::init(Log::Level _level, const QString& _filePath)
{
    m_logLevel = _level;

    if (!_filePath.isEmpty()) {
        const QFileInfo logFileInfo(_filePath);
        if (logFileInfo.exists() && logFileInfo.size() > 0) {
            info("Previous log file will be truncated");
        }

        const bool isDirCreated = QDir::root().mkpath(logFileInfo.absolutePath());
        if (!isDirCreated) {
            warning("Can't create folder \"%1\" for saving log file", _filePath);
            return;
        }

        m_logFile.setFileName(_filePath);
        const auto isFileOpened = m_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if (!isFileOpened) {
            warning("Can't open file \"%1\" to writing log. Error is \"%2\"", _filePath,
                    m_logFile.errorString());
            return;
        }
    }

    info("Logger initialized with \"%1\" level and \"%2\" log file path",
         QVariant::fromValue(_level).toString(), _filePath);
}

void Log::message(const QString& _message, Level _logLevel)
{
    if (_logLevel < m_logLevel) {
        return;
    }

    const QString time = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss.zzz");
    const std::map<Log::Level, QString> level2String = {
        { Log::Level::Debug, "D" },
        { Log::Level::Info, "I" },
        { Log::Level::Warning, "W" },
        { Log::Level::Critical, "C" },
    };
    const auto logEntry = QString("%1 [%2] %3").arg(time, level2String.at(_logLevel), _message);

    std::cout << logEntry.toStdString() << std::endl;

    if (m_logFile.isOpen()) {
        m_logFile.write(logEntry.toUtf8());
        m_logFile.write("\r\n");
    }
}
