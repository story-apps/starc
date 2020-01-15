#include "project_plugins_builder.h"

#include <interfaces/management_layer/i_document_manager.h>

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
    const QHash<QString, QVector<ProjectPluginsBuilder::EditorInfo>> kDocumentToEditors
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

class ProjectPluginsBuilder::Implementation
{
public:
    /**
     * @brief Активировать плагин заданного типа указанной моделью
     */
    QWidget* activatePlugin(const QString& _mimeType, BusinessLayer::AbstractModel* _model);

    /**
     * @brief Получить виджет плагина по заданному типу
     */
    QWidget* plugin(const QString& _mimeType, bool _fromCache = true) const;


    /**
     * @brief Загруженные плагины
     */
    mutable QHash<QString, ManagementLayer::IDocumentManager*> plugins;
};

QWidget* ProjectPluginsBuilder::Implementation::activatePlugin(const QString& _mimeType,
    BusinessLayer::AbstractModel* _model)
{
    if (!plugins.contains(_mimeType)) {
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

        auto plugin = qobject_cast<ManagementLayer::IDocumentManager*>(pluginObject);
        plugins.insert(_mimeType, plugin);
    }

    auto plugin = plugins.value(_mimeType);
    if (plugin == nullptr) {
        return nullptr;
    }

    plugin->setModel(_model);

    return plugin->view();
}


// ****


ProjectPluginsBuilder::ProjectPluginsBuilder()
    : d(new Implementation)
{
}

ProjectPluginsFactory::~ProjectPluginsFactory() = default;

QVector<ProjectPluginsBuilder::EditorInfo> ProjectPluginsBuilder::editorsInfoFor(const QString& _documentMimeType) const
{
    return kDocumentToEditors.value(_documentMimeType);
}

QString ProjectPluginsBuilder::navigatorMimeTypeFor(const QString& _editorMimeType) const
{
    return kEditorToNavigator.value(_editorMimeType);
}

QWidget* ProjectPluginsBuilder::activateView(const QString& _viewMimeType, BusinessLayer::AbstractModel* _model)
{
    return d->activatePlugin(_viewMimeType, _model);
}

} // namespace ManagementLayer
