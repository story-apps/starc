#include <interfaces/management_layer/i_application_manager.h>

#include <QApplication>
#include <QDir>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QDebug>

using ManagementLayer::IApplicationManager;

/**
 * @brief Загрузить менеджер приложения
 */
IApplicationManager* loadApplicationManager()
{
    //
    // Смотрим папку с данными приложения на компе
    // NOTE: В Debug-режим работает с папкой сборки приложения
    //
    const QString pluginsDirName = "plugins";
    QDir pluginsDir(
#ifndef QT_NO_DEBUG
                QApplication::applicationDirPath()
#else
                QStandardPaths::writableLocation(QStandardPaths::DataLocation)
#endif
                );

#if !defined(QT_NO_DEBUG) && defined(Q_OS_MAC)
    pluginsDir.cdUp();
    pluginsDir.cdUp();
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
            QFile::copy(installedPluginsDir.absoluteFilePath(file), pluginsDir.absoluteFilePath(file));
        }
    }
    //
    // Если плагины есть и если есть обновления
    //
    else {
        //
        // ... корректируем названия файлов для использования обновлённых версий
        //
        for (const QFileInfo &fileName : pluginsDir.entryInfoList({ "*.update" }, QDir::Files)) {
            QFile::rename(fileName.absoluteFilePath(), fileName.absoluteFilePath().remove(".update"));
        }
    }

    //
    // Подгружаем плагин
    //
    const QStringList libCorePluginEntries = pluginsDir.entryList({ "libcoreplugin*.*" }, QDir::Files);
    for (const QString &fileName : libCorePluginEntries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            return qobject_cast<ManagementLayer::IApplicationManager *>(plugin);
        }
    }

    return nullptr;
}

/**
 * @brief Погнали!
 */
int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("starc");
    application.setOrganizationName("DimkaNovikov labs.");
    application.setOrganizationDomain("dimkanovikov.pro");

    IApplicationManager* applicationManager = loadApplicationManager();
    if (applicationManager == nullptr) {
        return 1;
    }

    applicationManager->exec();
    return application.exec();
}
