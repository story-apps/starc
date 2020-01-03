#include "project_plugins_factory.h"

#include <interfaces/ui/i_document_plugin.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QHash>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QWidget>


namespace ManagementLayer
{

namespace {
    /**
     * @brief Карта соотвествия майм-типов редактора к навигатору
     */
    const QHash<QString, QString> kEditorToNavigator
    = {{ "application/x-starc/editor/screenplay/text", "application/x-starc/navigator/screenplay/structure" },
       { "application/x-starc/editor/screenplay/cards", "application/x-starc/navigator/screenplay/structure" }};

    /**
     * @brief Карта соответствия майм-типов документа к редакторам
     */
    const QHash<QString, QVector<ProjectPluginsFactory::EditorInfo>> kDocumentToEditors
    = {{ "application/x-starc/document/project", {{ "application/x-starc/editor/project/information", "\uf2fd" },
                                                  { "application/x-starc/editor/project/collaborators", "\ufb34" }}},
       { "application/x-starc/document/screenplay", {{ "application/x-starc/editor/screenplay", "" }}},
       { "application/x-starc/document/screenplay/title-page", {{ "application/x-starc/editor/screenplay/title-page", "" }}},
       { "application/x-starc/document/screenplay/logline", {{ "application/x-starc/editor/screenplay/logline", "" }}},
       { "application/x-starc/document/screenplay/synopsis", {{ "application/x-starc/editor/text", "" }}},
       { "application/x-starc/document/screenplay/text", {{ "application/x-starc/editor/screenplay/text", "\uf9ec" },
                                                          { "application/x-starc/editor/screenplay/cards", "\uf554" }}},
       { "application/x-starc/document/screenplay/outline", {{ "application/x-starc/editor/screenplay/text", "\uf9ec" },
                                                             { "application/x-starc/editor/screenplay/cards", "\uf554" }}}};

    /**
      * @brief Карта соответсвий майм-типов навигаторов/редакторов к названиям библиотек с плагинами
      * @note Нужно это для того, чтобы не прогружать каждый раз плагин со всеми зависимостями лишь для
      *       извлечения майм-типа навигатора/редактора, а подгружать точечно только то, что нужно
      */
    const QHash<QString, QString> kMimeToPlugin
    = {{ "application/x-starc/editor/project/information", "libprojectinformationplugin*.*" },
       { "application/x-starc/navigator/screenplay/structure", "libscreenplaystructureplugin*.*" },
       { "application/x-starc/editor/screenplay", "libscreenplayplugin*.*" },
       { "application/x-starc/editor/screenplay/title-page", "libscreenplaytitlepageplugin*.*" },
       { "application/x-starc/editor/screenplay/logline", "libscreenplayloglineplugin*.*" },
       { "application/x-starc/editor/screenplay/synopsis", "libtextplugin*.*" },
       { "application/x-starc/editor/screenplay/text", "libscreenplaytextplugin*.*" },
       { "application/x-starc/editor/screenplay/cards", "libscreenplaycardsplugin*.*" }};
}

class ProjectPluginsFactory::Implementation
{
public:
    /**
     * @brief Получить виджет плагина по заданному типу
     */
    QWidget* plugin(const QString& _mimeType, bool _fromCache = true) const;


    /**
     * @brief Загруженные плагины
     */
    mutable QHash<QString, Ui::IDocumentPlugin*> loadedPlugins;

    /**
     * @brief Загруженные виджеты
     */
    mutable QHash<QString, QWidget*> loadedWidgets;
};

QWidget* ProjectPluginsFactory::Implementation::plugin(const QString& _mimeType, bool _fromCache) const
{
    if (!loadedPlugins.contains(_mimeType)) {
        //
        // TODO: загрузка и соединение
        //

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
        // Если папки с плагинами нет, идём лесом
        //
        if (!pluginsDir.cd(pluginsDirName)) {
            return nullptr;
        }

        //
        // Подгружаем плагин
        //
        const QStringList libCorePluginEntries = pluginsDir.entryList({ kMimeToPlugin.value(_mimeType) }, QDir::Files);
        if (libCorePluginEntries.isEmpty()) {
            qCritical() << "Plugin isn't found for mime-type:" << _mimeType;
            return nullptr;
        }
        if (libCorePluginEntries.size() > 1) {
            qCritical() << "Found more than 1 plugins for mime-type:" << _mimeType;
            return nullptr;
        }

        const auto pluginPath = libCorePluginEntries.first();
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(pluginPath));
        QObject *pluginObject = pluginLoader.instance();
        if (pluginObject == nullptr) {
            qDebug() << pluginLoader.errorString();
        }

        Ui::IDocumentPlugin* plugin = qobject_cast<Ui::IDocumentPlugin*>(pluginObject);
        loadedPlugins.insert(_mimeType, plugin);
    }

    auto plugin = loadedPlugins.value(_mimeType);
    if (plugin == nullptr) {
        return nullptr;
    }

    //
    // Если нужен не из кэша, то просто создаём новый
    //
    if (!_fromCache) {
        return plugin->createWidget();
    }

    //
    // А если нужен из кэша, то проверяем если он там,
    //
    if (!loadedWidgets.contains(_mimeType)) {
        //
        // ... если нет, добавляем
        //
        loadedWidgets.insert(_mimeType, plugin->createWidget());
    }
    //
    // ... и возвращаем его
    //
    return loadedWidgets.value(_mimeType);
}


// ****


ProjectPluginsFactory::ProjectPluginsFactory()
    : d(new Implementation)
{
}

ProjectPluginsFactory::~ProjectPluginsFactory() = default;

QString ProjectPluginsFactory::navigatorFor(const QString& _editorMimeType) const
{
    return kEditorToNavigator.value(_editorMimeType);
}

QWidget* ProjectPluginsFactory::navigator(const QString& _mimeType) const
{
    return d->plugin(_mimeType);
}

QVector<ProjectPluginsFactory::EditorInfo> ProjectPluginsFactory::viewsFor(const QString& _documentMimeType) const
{
    return kDocumentToEditors.value(_documentMimeType);
}

QWidget* ProjectPluginsFactory::view(const QString& _mimeType) const
{
    return d->plugin(_mimeType);
}

} // namespace ManagementLayer
