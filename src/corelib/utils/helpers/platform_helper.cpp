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

using fnAllowDarkModeForWindow = BOOL(WINAPI*)(HWND hWnd, BOOL allow);
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA*);

static void setTitleBarThemeImpl(HWND hwnd, bool _isLight)
{
    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hUxtheme) {
        return;
    }

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) {
        return;
    }

    fnAllowDarkModeForWindow AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
    fnSetPreferredAppMode SetPreferredAppMode
        = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    fnSetWindowCompositionAttribute SetWindowCompositionAttribute
        = reinterpret_cast<fnSetWindowCompositionAttribute>(
            GetProcAddress(hUser32, "SetWindowCompositionAttribute"));
    if (!AllowDarkModeForWindow || !SetPreferredAppMode || !SetWindowCompositionAttribute) {
        return;
    }

    SetPreferredAppMode(_isLight ? Default : AllowDark);
    BOOL dark = _isLight ? FALSE : TRUE;
    AllowDarkModeForWindow(hwnd, dark);
    WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
    SetWindowCompositionAttribute(hwnd, &data);
}
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
