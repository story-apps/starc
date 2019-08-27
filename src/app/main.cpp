#include <interfaces/management_layer/i_application_manager.h>

#include <QApplication>
#include <QDir>
#include <QPluginLoader>
#include <QStandardPaths>

using ManagementLayer::IApplicationManager;

/**
 * @brief Загрузить менеджер приложения
 */
IApplicationManager* loadApplicationManager();


/**
 * @brief Погнали!
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("starc");
    a.setOrganizationName("DimkaNovikov labs.");
    a.setOrganizationDomain("dimkanovikov.pro");

    IApplicationManager* applicationManager = loadApplicationManager();
    if (applicationManager == nullptr) {
        return 1;
    }

    applicationManager->exec();
    return a.exec();
}

IApplicationManager* loadApplicationManager()
{
    //
    // Смотрим папку с данными приложения на компе
    //
    const QString pluginsDirName = "plugins";
    auto s = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir pluginsDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

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
        installedPluginsDir.cd("plugins");
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
    const QStringList lidCorePluginEntries = pluginsDir.entryList({ "libcoreplugin.*" }, QDir::Files);
    for (const QString &fileName : lidCorePluginEntries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            return qobject_cast<ManagementLayer::IApplicationManager *>(plugin);
        }
    }

    return nullptr;
}
