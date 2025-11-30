#include "crashpad_paths.h"

#include <QStandardPaths>

#if defined(Q_OS_WIN)
#define NOMINMAX
#include <filesystem>
#include <windows.h>
#endif

#if defined(Q_OS_MAC)
#include <mach-o/dyld.h>
#endif

#if defined(Q_OS_LINUX)
#include <unistd.h>
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

namespace {

QString getExecutableDir()
{
#if defined(Q_OS_MAC)
    unsigned int bufferSize = 512;
    std::vector<char> buffer(bufferSize + 1);

    if (_NSGetExecutablePath(&buffer[0], &bufferSize)) {
        buffer.resize(bufferSize);
        _NSGetExecutablePath(&buffer[0], &bufferSize);
    }

    char* lastForwardSlash = strrchr(&buffer[0], '/');
    if (lastForwardSlash == NULL)
        return NULL;
    *lastForwardSlash = 0;

    return &buffer[0];
#elif defined(Q_OS_WINDOWS)
    HMODULE hModule = GetModuleHandleW(NULL);
    WCHAR path[MAX_PATH];
    DWORD retVal = GetModuleFileNameW(hModule, path, MAX_PATH);
    if (retVal == 0)
        return NULL;

    wchar_t* lastBackslash = wcsrchr(path, '\\');
    if (lastBackslash == NULL)
        return NULL;
    *lastBackslash = 0;

    return QString::fromWCharArray(path);
#elif defined(Q_OS_LINUX)
    char pBuf[FILENAME_MAX];
    int len = sizeof(pBuf);
    int bytes = MIN(readlink("/proc/self/exe", pBuf, len), len - 1);
    if (bytes >= 0) {
        pBuf[bytes] = '\0';
    }

    char* lastForwardSlash = strrchr(&pBuf[0], '/');
    if (lastForwardSlash == NULL)
        return NULL;
    *lastForwardSlash = '\0';

    return QString::fromStdString(pBuf);
#else
#error getExecutableDir not implemented on this platform
#endif
}
} // namespace

CrashpadPaths::CrashpadPaths()
    : m_exeDir(getExecutableDir())
{
}

QString CrashpadPaths::getAttachmentPath()
{
#if defined(Q_OS_MACOS)
    return m_exeDir + "/../Resources/attachment.txt";
#elif defined(Q_OS_WINDOWS)
    return m_exeDir + "\\..\\attachment.txt";
#elif defined(Q_OS_LINUX)
    return m_exeDir + "/attachment.txt";
#else
#error getAttachmentPath() not implemented on this platform
#endif
}

QString CrashpadPaths::getHandlerPath()
{
#if defined(Q_OS_MAC)
    return m_exeDir + "/crashpad/crashpad_handler";
#elif defined(Q_OS_WINDOWS)
    return m_exeDir + "\\crashpad\\crashpad_handler.exe";
#elif defined(Q_OS_LINUX)
    return m_exeDir + "/crashpad/crashpad_handler";
#else
#error getHandlerPath not implemented on this platform
#endif
}

QString CrashpadPaths::getReportsPath()
{
#if defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashpad";
#elif defined(Q_OS_WINDOWS)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashpad";
#elif defined(Q_OS_LINUX)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashpad";
#else
#error getReportsPath not implemented on this platform
#endif
}

QString CrashpadPaths::getMetricsPath()
{
#if defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashpad";
#elif defined(Q_OS_WINDOWS)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashpad";
#elif defined(Q_OS_LINUX)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashpad";
#else
#error getMetricsPath not implemented on this platform
#endif
}

#if defined(Q_OS_UNIX)
std::string CrashpadPaths::getPlatformString(QString string)
{
    return string.toStdString();
}
#elif defined(Q_OS_WINDOWS)
std::wstring CrashpadPaths::getPlatformString(QString string)
{
    return string.toStdWString();
}
#else
#error getPlatformString not implemented on this platform
#endif
