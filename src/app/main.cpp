#include "application.h"

#include <interfaces/management_layer/i_application_manager.h>

#include <QBreakpadHandler.h>
#include <QDebug>
#include <QDir>
#include <QPluginLoader>
#include <QStandardPaths>

using ManagementLayer::IApplicationManager;

/**
 * @brief Загрузить менеджер приложения
 */
QObject* loadApplicationManager()
{
    //
    // Смотрим папку с данными приложения на компе
    // NOTE: В Debug-режим работает с папкой сборки приложения
    //
    // TODO: Когда дорастём включить этот функционал, плюс продумать, как быть в режиме разработки
    //
    const QString pluginsDirName = "plugins";
    QDir pluginsDir(
        //#ifndef QT_NO_DEBUG
        QApplication::applicationDirPath()
        //#else
        //                QStandardPaths::writableLocation(QStandardPaths::DataLocation)
        //#endif
    );

#if defined(Q_OS_MAC)
    pluginsDir.cdUp();
#endif

    //
    // Если там нет плагинов приложения
    //
    if (!pluginsDir.cd(pluginsDirName)) {
        //
        // ... создаём локальную папку с плагинами
        //
        pluginsDir.mkpath(pluginsDir.absoluteFilePath(pluginsDirName));
        pluginsDir.cd(pluginsDirName);
        //
        // ... и копируем туда все плагины из папки, в которую установлено приложение
        //
        QDir installedPluginsDir(QApplication::applicationDirPath());
        installedPluginsDir.cd(pluginsDirName);
        for (const auto& file : installedPluginsDir.entryList(QDir::Files)) {
            QFile::copy(installedPluginsDir.absoluteFilePath(file),
                        pluginsDir.absoluteFilePath(file));
        }
    }
    //
    // Если плагины есть и если есть обновления
    //
    else {
        //
        // ... корректируем названия файлов для использования обновлённых версий
        //
        for (const QFileInfo& fileName : pluginsDir.entryInfoList({ "*.update" }, QDir::Files)) {
            QFile::rename(fileName.absoluteFilePath(),
                          fileName.absoluteFilePath().remove(".update"));
        }
    }

    //
    // Подгружаем плагин
    //
    const QString extensionFilter =
#ifdef Q_OS_WIN
        ".dll";
#else
        "";
#endif
    const QStringList libCorePluginEntries
        = pluginsDir.entryList({ "*coreplugin*" + extensionFilter }, QDir::Files);
    if (libCorePluginEntries.isEmpty()) {
        qCritical() << "Core plugin isn't found";
        return nullptr;
    }
    if (libCorePluginEntries.size() > 1) {
        qCritical() << "Found more than 1 core plugins";
        return nullptr;
    }

    const auto pluginPath = libCorePluginEntries.first();
    QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(pluginPath));
    QObject* plugin = pluginLoader.instance();
    if (plugin == nullptr) {
        qDebug() << pluginLoader.errorString();
    }

    return plugin;
}

/**
 * @brief Погнали!
 */
int main(int argc, char* argv[])
{
    //
    // Инициилизируем приложение
    //
    Application application(argc, argv);

    //
    // Загружаем менеджера приложения
    //
    auto applicationManager = loadApplicationManager();
    if (applicationManager == nullptr) {
        return 1;
    }

    //
    // Настраиваем сборщик крашдампов
    //
    const auto crashReportsFolderPath = QString("%1/%2").arg(
        QStandardPaths::writableLocation(QStandardPaths::DataLocation), "crashreports");
    QBreakpadInstance.init(crashReportsFolderPath);

    //
    // Устанавливаем менеджера в приложение и запускаемся
    //
    application.setApplicationManager(applicationManager);
    application.startUp();
    return application.exec();
}
