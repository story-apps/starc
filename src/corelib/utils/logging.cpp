#include "logging.h"

#include <QDateTime>
#include <QDir>
#include <QVariant>

#include <iostream>


Log::Level Log::s_logLevel = Log::Level::Warning;
QFile Log::s_logFile;

void Log::init(Log::Level _level, const QString& _filePath)
{
    qInstallMessageHandler(qtOutputHandler);

    s_logLevel = _level;

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

        s_logFile.setFileName(_filePath);
        const auto isFileOpened = s_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if (!isFileOpened) {
            warning("Can't open file \"%1\" to writing log. Error is \"%2\"", _filePath,
                    s_logFile.errorString());
            return;
        }
    }

    trace("Logger initialized with \"%1\" level and \"%2\" log file path",
          QVariant::fromValue(_level).toString(), _filePath);
}

QString Log::logFilePath()
{
    return s_logFile.fileName();
}

void Log::message(const QString& _message, Level _logLevel)
{
    if (_logLevel < s_logLevel) {
        return;
    }

    const QString time = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss.zzz");
    const std::map<Log::Level, QString> level2String = { { Log::Level::Debug, "D" },
                                                         { Log::Level::Info, "I" },
                                                         { Log::Level::Warning, "W" },
                                                         { Log::Level::Critical, "C" },
                                                         { Log::Level::Fatal, "F" } };
    const auto logEntry = QString("%1 [%2] %3").arg(time, level2String.at(_logLevel), _message);

    std::cout << logEntry.toStdString() << std::endl;

    if (s_logFile.isOpen()) {
        s_logFile.write(logEntry.toUtf8());
        s_logFile.write("\r\n");
        s_logFile.flush();
    }
}

void Log::qtOutputHandler(QtMsgType _type, const QMessageLogContext& _context,
                          const QString& _message)
{
    const auto message = (qstrlen(_context.file) > 0 && qstrlen(_context.function) > 0)
        ? QString("%1 (%2:%3, %4)")
              .arg(_message, _context.file, QString::number(_context.line),
                   QString(_context.function).right(100))
        : _message;
    switch (_type) {
    case QtDebugMsg:
        debug(message);
        break;
    case QtInfoMsg:
        info(message);
        break;
    case QtWarningMsg:
        warning(message);
        break;
    case QtCriticalMsg:
        critical(message);
        break;
    case QtFatalMsg:
        fatal(message);
        abort();
    }
}
