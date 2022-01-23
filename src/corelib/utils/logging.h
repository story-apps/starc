#pragma once

#include <QFile>

#include <corelib_global.h>


class CORE_LIBRARY_EXPORT Log
{
    Q_GADGET

public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Critical,
        Fatal,
    };
    Q_ENUM(Level);

    /**
     * @brief Set displaying log level
     */
    static void init(Level _logLevel, const QString& _filePath);

    /**
     * @brief Путь к текущему файлу логов
     */
    static QString logFilePath();

    /**
     * @brief Directly outputs message
     */
    static void message(const QString& _message, Level _logLevel);

    /**
     * @brief All public methods below are used for create log message with corresponding level
     */

    template<typename... Args>
    static void debug(const QString& _message, Args... _args)
    {
        if (s_logLevel > Level::Debug) {
            return;
        }

        message(constructMessage(_message, _args...), Level::Debug);
    }

    template<typename... Args>
    static void info(const QString& _message, Args... _args)
    {
        if (s_logLevel > Level::Info) {
            return;
        }

        message(constructMessage(_message, _args...), Level::Info);
    }

    template<typename... Args>
    static void warning(const QString& _message, Args... _args)
    {
        if (s_logLevel > Level::Warning) {
            return;
        }

        message(constructMessage(_message, _args...), Level::Warning);
    }

    template<typename... Args>
    static void critical(const QString& _message, Args... _args)
    {
        if (s_logLevel > Level::Critical) {
            return;
        }

        message(constructMessage(_message, _args...), Level::Critical);
    }

    template<typename... Args>
    static void fatal(const QString& _message, Args... _args)
    {
        if (s_logLevel > Level::Fatal) {
            return;
        }

        message(constructMessage(_message, _args...), Level::Fatal);
    }

    static void qtOutputHandler(QtMsgType _type, const QMessageLogContext& _context,
                                const QString& _message);

private:
    /**
     * @brief These methods are used for apply args to QString
     */
    template<typename... Args>
    static QString constructMessage(const QString& _message, Args&&... _args)
    {
        return constructMessage(_message.arg(_args...));
    }

    template<typename Arg>
    static QString constructMessage(const QString& _message, Arg _arg)
    {
        return _message.arg(_arg);
    }

    static QString constructMessage(const QString& _message)
    {
        return _message;
    }

private:
    /**
     * @brief Loging level
     */
    static Level s_logLevel;

    /**
     * @brief File with log
     */
    static QFile s_logFile;
};
