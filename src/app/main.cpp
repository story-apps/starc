#include "application.h"

#include <interfaces/management_layer/i_application_manager.h>

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
    QObject *plugin = pluginLoader.instance();
    if (plugin == nullptr) {
        qDebug() << pluginLoader.errorString();
    }

    return plugin;
}

/**
 * @brief Погнали!
 */
int main(int argc, char *argv[])
{
    Application application(argc, argv);


    auto applicationManager = loadApplicationManager();
    if (applicationManager == nullptr) {
        return 1;
    }

    application.setApplicationManager(applicationManager);
    application.startUp();
    return application.exec();
}
