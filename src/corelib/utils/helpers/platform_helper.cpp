#include "platform_helper.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QUrl>
#include <QWidget>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#endif

namespace {

#ifdef Q_OS_WIN
//
// Код взят и адаптирован отсюда https://github.com/komiyamma/win32-darkmode/
//
// clang-format off
enum IMMERSIVE_HC_CACHE_MODE
{
    IHCM_USE_CACHED_VALUE,
    IHCM_REFRESH
};

enum PreferredAppMode {
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max,
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27,
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

using fnRtlGetNtVersionNumbers = void (WINAPI *)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL (WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
using fnShouldAppsUseDarkMode = bool (WINAPI *)(); // ordinal 132
using fnAllowDarkModeForWindow = bool (WINAPI *)(HWND hWnd, bool allow); // ordinal 133
using fnAllowDarkModeForApp = bool (WINAPI *)(bool allow); // ordinal 135, in 1809
using fnFlushMenuThemes = void (WINAPI *)(); // ordinal 136
using fnRefreshImmersiveColorPolicyState = void (WINAPI *)(); // ordinal 104
using fnIsDarkModeAllowedForWindow = bool (WINAPI *)(HWND hWnd); // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool (WINAPI *)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
using fnOpenNcThemeData = HTHEME(WINAPI *)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool (WINAPI *)(); // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode (WINAPI *)(PreferredAppMode appMode); // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool (WINAPI *)(); // ordinal 139

constexpr bool CheckBuildNumber(DWORD buildNumber)
{
    return (buildNumber == 17763 || // 1809
            buildNumber == 18362 || // 1903
            buildNumber == 18363 || // 1909
            buildNumber == 19041 || // 2004
            buildNumber >= 19042); // over 2009
}

void setTitleBarThemeImpl(HWND hwnd, bool _isLight)
{
    auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
    if (!RtlGetNtVersionNumbers) {
        return;
    }

    DWORD major, minor, buildNumber = 0;
    RtlGetNtVersionNumbers(&major, &minor, &buildNumber);
    buildNumber &= ~0xF0000000;
    if (major < 10 || !CheckBuildNumber(buildNumber)) {
        return;
    }

    auto hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hUxtheme) {
        return;
    }

    auto _OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
    auto _RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
    auto  _ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
    auto  _AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

    auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
    fnSetPreferredAppMode _SetPreferredAppMode = nullptr;
    if (buildNumber < 18362) {
        _AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
    } else {
        _SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);
    }

    auto _IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

    if (_OpenNcThemeData &&
        _RefreshImmersiveColorPolicyState &&
        _ShouldAppsUseDarkMode &&
        _AllowDarkModeForWindow &&
        (_AllowDarkModeForApp || _SetPreferredAppMode) &&
        _IsDarkModeAllowedForWindow) {
        _RefreshImmersiveColorPolicyState();

        BOOL dark = _isLight ? FALSE : TRUE;
        if (_AllowDarkModeForApp) {
            _AllowDarkModeForApp(dark);
        } else if (_SetPreferredAppMode) {
            _SetPreferredAppMode(_isLight ? Default : AllowDark);
        }

        auto _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        if (buildNumber < 18362) {
            SetPropW(hwnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
        } else if (_SetWindowCompositionAttribute) {
            WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
            _SetWindowCompositionAttribute(hwnd, &data);
        }
    }
}
// clang-format on
#endif

bool openPath(const QString& _path)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(_path));
}

} // namespace


void PlatformHelper::initConsoleOutput()
{
#ifdef Q_OS_WIN
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif
}

void PlatformHelper::setTitleBarTheme(QWidget* _window, bool _isLightTheme)
{
#ifdef Q_OS_WIN
    setTitleBarThemeImpl(reinterpret_cast<HWND>(_window->winId()), _isLightTheme);
#else
    Q_UNUSED(_window)
    Q_UNUSED(_isLightTheme)
#endif
}

QString PlatformHelper::systemSavebleFileName(const QString& _fileName)
{
    QString result = _fileName;
#ifdef Q_OS_WIN
    result = result.replace("\"", "_").replace(":", "_").replace("?", "_");
#endif
    return result;
}

bool PlatformHelper::showInGraphicalShell(const QString& _path)
{
    const QFileInfo fileInfo(_path);
    // Mac, Windows support folder or file.
#ifdef Q_OS_WINDOWS
    const auto explorer = QStandardPaths::findExecutable(QLatin1String("explorer.exe"));
    if (explorer.isEmpty()) {
        return false;
    }
    QStringList param;
    if (!fileInfo.isDir())
        param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(fileInfo.canonicalFilePath());
    return QProcess::startDetached(explorer, param);
#elif defined(Q_OS_MACOS)
    return QProcess::startDetached("/usr/bin/open", { "-R", fileInfo.canonicalFilePath() });
#else
    if (fileInfo.exists()) {
        QProcess process;
        QString output;
        process.start("xdg-mime", { "query", "default", "inode/directory" });
        process.waitForFinished();
        output = process.readLine().simplified();
        if (output == "dolphin.desktop" || output == "org.kde.dolphin.desktop")
            return process.startDetached("dolphin", { "--select", _path });
        else if (output == "nautilus.desktop" || output == "org.gnome.Nautilus.desktop"
                 || output == "nautilus-folder-handler.desktop")
            return process.startDetached("nautilus", { "--no-desktop", _path });
        else if (output == "caja-folder-handler.desktop")
            return process.startDetached("caja", { "--no-desktop", _path });
        else if (output == "nemo.desktop")
            return process.startDetached("nemo", { "--no-desktop", _path });
        else if (output == "kfmclient_dir.desktop")
            return process.startDetached("konqueror", { "--select", _path });
        else
            return openPath(_path.left(_path.lastIndexOf("/")));
    } else {
        // If the item to select doesn't exist, try to open its parent
        return openPath(_path.left(_path.lastIndexOf("/")));
    }
#endif
}
