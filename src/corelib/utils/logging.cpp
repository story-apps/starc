#include "logging.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QVariant>

#include <iostream>

namespace {
static QFile s_logFile;
static QElapsedTimer s_dayTimer;
} // namespace

Log::Level Log::s_logLevel = Log::Level::Warning;

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

        s_dayTimer.start();
    }

    trace("Logger initialized with \"%1\" level and \"%2\" log file path",
          QVariant::fromValue(_level).toString(), _filePath);
}

void Log::printBuildInfo()
{
    info("Corelib build info - hash: %1; build date: %2",
         //
         // git commit from corelib.pro
         //
         CORE_LIBRARY_GIT_COMMIT_HASH,
         //
         // build datetime from corelib.pro
         //
         CORE_LIBRARY_BUILD_TIMESTAMP);
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

    if (s_logFile.isOpen() && s_dayTimer.hasExpired(24 * 60 * 60 * 1000)) {
        const auto logPath = s_logFile.fileName();
        s_logFile.close();
        s_logFile.rename(QDate::currentDate().addDays(-1).toString(Qt::ISODate) + ".log");
        init(s_logLevel, logPath);
    }

    const QString time = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss.zzz");
    const std::map<Log::Level, QString> level2String = {
        { Log::Level::Trace, "T" },   { Log::Level::Debug, "D" },    { Log::Level::Info, "I" },
        { Log::Level::Warning, "W" }, { Log::Level::Critical, "C" }, { Log::Level::Fatal, "F" },
    };
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
        ? QString("%1 (...%2:%3, %4)")
              .arg(_message, QString(_context.file).right(30), QString::number(_context.line),
                   _context.function)
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
