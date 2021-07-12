#include "project_plugins_builder.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QHash>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QWidget>


namespace ManagementLayer {

namespace {

/**
 * @brief Майм-типы плагинов
 */
const QString kSimpleTextEditorMime = QStringLiteral("application/x-starc/editor/text/text");
const QString kSimpleTextNavigatorMime = QStringLiteral("application/x-starc/navigator/text/text");
const QString kScreenplayTitlePageEditorMime
    = QStringLiteral("application/x-starc/editor/screenplay/title-page");
const QString kScreenplayTextEditorMime
    = QStringLiteral("application/x-starc/editor/screenplay/text");
const QString kScreenplayTextNavigatorMime
    = QStringLiteral("application/x-starc/navigator/screenplay/text");
const QString kScreenplayStatisticsViewMime
    = QStringLiteral("application/x-starc/view/screenplay/statistics");
const QString kScreenplayStatisticsNavigatorMime
    = QStringLiteral("application/x-starc/navigator/screenplay/statistics");

/**
 * @brief Карта соотвествия майм-типов редактора к навигатору
 */
const QHash<QString, QString> kEditorToNavigator
    = { { kSimpleTextEditorMime, kSimpleTextNavigatorMime },
        { kScreenplayTextEditorMime, kScreenplayTextNavigatorMime },
        { "application/x-starc/editor/screenplay/cards", kScreenplayTextNavigatorMime },
        { kScreenplayStatisticsViewMime, kScreenplayStatisticsNavigatorMime } };

/**
 * @brief Карта соответствия майм-типов документа к редакторам
 */
const QHash<QString, QVector<ProjectPluginsBuilder::EditorInfo>> kDocumentToEditors
    = { { "application/x-starc/document/project",
          { { "application/x-starc/editor/project/information", u8"\U000f02fd" },
            { "application/x-starc/editor/project/collaborators", u8"\U000f0b58" } } },
        { "application/x-starc/document/screenplay",
          { { "application/x-starc/editor/screenplay/information", u8"\U000f02fd" },
            { "application/x-starc/editor/screenplay/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/screenplay/title-page",
          { { kScreenplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay/synopsis",
          { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay/treatment",
          { { "application/x-starc/editor/screenplay/treatment", u8"\U000f09ed" },
            { "application/x-starc/editor/screenplay/beatboard", u8"\U000f0554" },
            { "application/x-starc/editor/screenplay/treatmentcards", u8"\U000f0554" } } },
        { "application/x-starc/document/screenplay/text",
          { { kScreenplayTextEditorMime, u8"\U000f09ed" },
            { "application/x-starc/editor/screenplay/cards", u8"\U000f0554" } } },
        { "application/x-starc/document/screenplay/statistics",
          { { kScreenplayStatisticsViewMime, u8"\U000f0127" } } },
        { "application/x-starc/document/character",
          { { "application/x-starc/editor/character/information", u8"\U000f02fd" } } },
        { "application/x-starc/document/location",
          { { "application/x-starc/editor/location/information", u8"\U000f02fd" } } },
        { "application/x-starc/document/folder", { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/text", { { kSimpleTextEditorMime, u8"\U000f09ed" } } } };

/**
 * @brief Карта соответсвий майм-типов навигаторов/редакторов к названиям библиотек с плагинами
 * @note Нужно это для того, чтобы не прогружать каждый раз плагин со всеми зависимостями лишь для
 *       извлечения майм-типа навигатора/редактора, а подгружать точечно только то, что нужно
 */
const QHash<QString, QString> kMimeToPlugin
    = { { "application/x-starc/editor/project/information", "*projectinformationplugin*" },
        { "application/x-starc/editor/screenplay/information", "*screenplayinformationplugin*" },
        { "application/x-starc/editor/screenplay/parameters", "*screenplayparametersplugin*" },
        { kScreenplayTitlePageEditorMime, "*screenplaytitlepageplugin*" },
        { "application/x-starc/editor/screenplay/treatment", "*screenplaytreatmentplugin*" },
        { "application/x-starc/editor/screenplay/treatment-cards",
          "*screenplaytreatmentcardsplugin*" },
        { kScreenplayTextEditorMime, "*screenplaytextplugin*" },
        { kScreenplayTextNavigatorMime, "*screenplaytextstructureplugin*" },
        { "application/x-starc/editor/screenplay/cards", "*screenplaycardsplugin*" },
        { kScreenplayStatisticsViewMime, "*screenplaystatisticsplugin*" },
        { kScreenplayStatisticsNavigatorMime, "*screenplaystatisticsstructureplugin*" },
        { "application/x-starc/editor/character/information", "*characterinformationplugin*" },
        { "application/x-starc/editor/location/information", "*locationinformationplugin*" },
        { kSimpleTextEditorMime, "*simpletextplugin*" },
        { kSimpleTextNavigatorMime, "*simpletextstructureplugin*" } };

} // namespace

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
     * @brief Загруженные плагины <mime, plugin>
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
        // TODO: Когда дорастём включить этот функционал, плюс продумать, как быть в режиме
        // разработки
        //
        const QString pluginsDirName = "plugins";
        QDir pluginsDir(
            //#ifndef QT_NO_DEBUG
            QApplication::applicationDirPath()
            //#else
            //                    QStandardPaths::writableLocation(QStandardPaths::DataLocation)
            //#endif
        );

#if defined(Q_OS_MAC)
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
        const QString extensionFilter =
#ifdef Q_OS_WIN
            ".dll";
#else
            "";
#endif
        const QStringList libCorePluginEntries = pluginsDir.entryList(
            { kMimeToPlugin.value(_mimeType) + extensionFilter }, QDir::Files);
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
        QObject* pluginObject = pluginLoader.instance();
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

ProjectPluginsBuilder::~ProjectPluginsBuilder() = default;

QVector<ProjectPluginsBuilder::EditorInfo> ProjectPluginsBuilder::editorsInfoFor(
    const QString& _documentMimeType) const
{
    return kDocumentToEditors.value(_documentMimeType);
}

QString ProjectPluginsBuilder::editorDescription(const QString& _editorMimeType) const
{
    const QHash<QString, QString> descriptions
        = { { "application/x-starc/editor/project/information",
              QApplication::translate("ProjectPluginsBuilder", "Information about project") },
            { "application/x-starc/editor/project/collaborators",
              QApplication::translate("ProjectPluginsBuilder", "Project collaborators") },
            { "application/x-starc/editor/screenplay/information",
              QApplication::translate("ProjectPluginsBuilder", "Information about screenplay") },
            { "application/x-starc/editor/screenplay/parameters",
              QApplication::translate("ProjectPluginsBuilder", "Screenplay parameters") },
            { "application/x-starc/editor/screenplay/title-page",
              QApplication::translate("ProjectPluginsBuilder", "Title page text") },
            { "application/x-starc/editor/text",
              QApplication::translate("ProjectPluginsBuilder", "Text") },
            { "application/x-starc/editor/screenplay/treatment",
              QApplication::translate("ProjectPluginsBuilder", "Treatment text") },
            { "application/x-starc/editor/screenplay/text",
              QApplication::translate("ProjectPluginsBuilder", "Screenplay text") },
            { "application/x-starc/editor/screenplay/cards",
              QApplication::translate("ProjectPluginsBuilder", "Cards") },
            { "application/x-starc/editor/screenplay/statistics",
              QApplication::translate("ProjectPluginsBuilder", "Statistics") } };
    return descriptions.value(_editorMimeType);
}

QString ProjectPluginsBuilder::navigatorMimeTypeFor(const QString& _editorMimeType) const
{
    return kEditorToNavigator.value(_editorMimeType);
}

QWidget* ProjectPluginsBuilder::activateView(const QString& _viewMimeType,
                                             BusinessLayer::AbstractModel* _model)
{
    return d->activatePlugin(_viewMimeType, _model);
}

void ProjectPluginsBuilder::bind(const QString& _viewMimeType, const QString& _navigatorMimeType)
{
    auto viewPlugin = d->plugins.value(_viewMimeType);
    Q_ASSERT(viewPlugin);

    auto navigatorPlugin = d->plugins.value(_navigatorMimeType);
    Q_ASSERT(navigatorPlugin);

    viewPlugin->bind(navigatorPlugin);
    navigatorPlugin->bind(viewPlugin);
}

void ProjectPluginsBuilder::reconfigureAll()
{
    for (auto& plugin : d->plugins) {
        plugin->reconfigure({});
    }
}

void ProjectPluginsBuilder::reconfigurePlugin(const QString& _mimeType,
                                              const QStringList& _changedSettingsKeys)
{
    auto plugin = d->plugins.find(_mimeType);
    if (plugin == d->plugins.end()) {
        return;
    }

    plugin.value()->reconfigure(_changedSettingsKeys);
}

void ProjectPluginsBuilder::reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys)
{
    reconfigurePlugin(kSimpleTextEditorMime, _changedSettingsKeys);
}

void ProjectPluginsBuilder::reconfigureSimpleTextNavigator()
{
    reconfigurePlugin(kSimpleTextNavigatorMime, {});
}

void ProjectPluginsBuilder::reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys)
{
    reconfigurePlugin(kScreenplayTextEditorMime, _changedSettingsKeys);
}

void ProjectPluginsBuilder::reconfigureScreenplayNavigator()
{
    reconfigurePlugin(kScreenplayTextNavigatorMime, {});
}

void ProjectPluginsBuilder::reset()
{
    for (auto& plugin : d->plugins) {
        plugin->saveSettings();
        plugin->setModel(nullptr);
    }
}

} // namespace ManagementLayer
