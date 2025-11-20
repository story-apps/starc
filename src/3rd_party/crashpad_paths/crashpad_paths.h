#pragma once

#include <QString>

class CrashpadPaths
{
    QString m_exeDir;

    public:
        CrashpadPaths();
        QString getAttachmentPath();
        QString getHandlerPath();
        QString getReportsPath();
        QString getMetricsPath();
        #if defined(Q_OS_UNIX)
            static std::string getPlatformString(QString string);
        #elif defined(Q_OS_WINDOWS)
            static std::wstring getPlatformString(QString string);
        #else
            #error getPlatformString not implemented on this platform
        #endif
};
