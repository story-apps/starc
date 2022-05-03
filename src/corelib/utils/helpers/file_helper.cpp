#include "file_helper.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QUrl>


namespace {
bool openPath(const QString& _path)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(_path));
}
} // namespace

QString FileHelper::systemSavebleFileName(const QString& _fileName)
{

    QString result = _fileName;
#ifdef Q_OS_WIN
    result = result.replace("\"", "_").replace(":", "_").replace("?", "_");
#endif
    return result;
}

bool FileHelper::showInGraphicalShell(const QString& _path)
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
    return QProcess::startDetached({ "/usr/bin/open", { "-R", fileInfo.canonicalFilePath() } });
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
