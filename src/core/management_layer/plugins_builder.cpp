#include "plugins_builder.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <interfaces/ui/i_document_view.h>

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
 * @brief Тип представления для активации из плагина
 */
enum ViewType {
    Primary,
    Secondary,
    Window,
};

// clang-format off
/**
 * @brief Майм-типы плагинов
 */
const QString kSimpleTextEditorMime = QStringLiteral("application/x-starc/editor/text/text");
const QString kSimpleTextNavigatorMime = QStringLiteral("application/x-starc/navigator/text/text");
//
const QString kScreenplayTitlePageEditorMime = QStringLiteral("application/x-starc/editor/screenplay/title-page");
const QString kScreenplayTreatmentEditorMime = QStringLiteral("application/x-starc/editor/screenplay/treatment");
const QString kScreenplayTreatmentNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/treatment");
const QString kScreenplayTextEditorMime = QStringLiteral("application/x-starc/editor/screenplay/text");
const QString kScreenplayTextNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/text");
const QString kScreenplayStatisticsViewMime = QStringLiteral("application/x-starc/view/screenplay/statistics");
const QString kScreenplayStatisticsNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/statistics");
//
const QString kComicBookTitlePageEditorMime = QStringLiteral("application/x-starc/editor/comicbook/title-page");
const QString kComicBookTextEditorMime = QStringLiteral("application/x-starc/editor/comicbook/text");
const QString kComicBookTextNavigatorMime = QStringLiteral("application/x-starc/navigator/comicbook/text");
const QString kComicBookStatisticsViewMime = QStringLiteral("application/x-starc/view/comicbook/statistics");
//
const QString kAudioplayInformationViewMime = QStringLiteral("application/x-starc/view/audioplay/information");
const QString kAudioplayParametersViewMime = QStringLiteral("application/x-starc/view/audioplay/parameters");
const QString kAudioplayTitlePageEditorMime = QStringLiteral("application/x-starc/editor/audioplay/title-page");
const QString kAudioplayTextEditorMime = QStringLiteral("application/x-starc/editor/audioplay/text");
const QString kAudioplayTextNavigatorMime = QStringLiteral("application/x-starc/navigator/audioplay/text");
const QString kAudioplayStatisticsViewMime = QStringLiteral("application/x-starc/view/audioplay/statistics");
//
const QString kStageplayInformationViewMime = QStringLiteral("application/x-starc/view/stageplay/information");
const QString kStageplayParametersViewMime = QStringLiteral("application/x-starc/view/stageplay/parameters");
const QString kStageplayTitlePageEditorMime = QStringLiteral("application/x-starc/editor/stageplay/title-page");
const QString kStageplayTextEditorMime = QStringLiteral("application/x-starc/editor/stageplay/text");
const QString kStageplayTextNavigatorMime = QStringLiteral("application/x-starc/navigator/stageplay/text");
const QString kStageplayStatisticsViewMime = QStringLiteral("application/x-starc/view/stageplay/statistics");

/**
 * @brief Карта соотвествия майм-типов редактора к навигатору
 */
const QHash<QString, QString> kEditorToNavigator
    = { { kSimpleTextEditorMime, kSimpleTextNavigatorMime },
        { kScreenplayTreatmentEditorMime, kScreenplayTreatmentNavigatorMime },
        { kScreenplayTextEditorMime, kScreenplayTextNavigatorMime },
        { "application/x-starc/editor/screenplay/cards", kScreenplayTextNavigatorMime },
//        { kScreenplayStatisticsViewMime, kScreenplayStatisticsNavigatorMime },
        { kComicBookTextEditorMime, kComicBookTextNavigatorMime },
        { kAudioplayTextEditorMime, kAudioplayTextNavigatorMime },
        { kStageplayTextEditorMime, kStageplayTextNavigatorMime },
      };

/**
 * @brief Карта соответствия майм-типов документа к редакторам
 */
const QHash<QString, QVector<PluginsBuilder::EditorInfo>> kDocumentToEditors
    = { { "application/x-starc/document/project",    { { "application/x-starc/editor/project/information", u8"\U000f02fd" },
                                                       { "application/x-starc/editor/project/collaborators", u8"\U000f0b58" } } },
        //
        { "application/x-starc/document/screenplay", { { "application/x-starc/editor/screenplay/information", u8"\U000f02fd" },
                                                       { "application/x-starc/editor/screenplay/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/screenplay/title-page", { { kScreenplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay/synopsis",   { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay/treatment",  { { kScreenplayTreatmentEditorMime, u8"\U000f09ed" },
                                                                  { "application/x-starc/editor/screenplay/beatboard", u8"\U000F13D2" },
                                                                  { "application/x-starc/editor/screenplay/treatmentcards", u8"\U000f0554" } } },
        { "application/x-starc/document/screenplay/text",       { { kScreenplayTextEditorMime, u8"\U000f09ed" },
                                                                  { "application/x-starc/editor/screenplay/cards", u8"\U000f0554" },
                                                                  { "application/x-starc/editor/screenplay/timeline", u8"\U000F066C" } } },
        { "application/x-starc/document/screenplay/statistics", { { kScreenplayStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/comicbook",  { { "application/x-starc/editor/comicbook/information", u8"\U000f02fd" },
                                                       { "application/x-starc/editor/comicbook/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/comicbook/title-page",  { { kComicBookTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/comicbook/synopsis",    { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/comicbook/text",        { { kComicBookTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/comicbook/statistics",  { { kComicBookStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/audioplay", { { kAudioplayInformationViewMime, u8"\U000f02fd" },
                                                       { kAudioplayParametersViewMime, u8"\U000f0493" } } },
        { "application/x-starc/document/audioplay/title-page",  { { kAudioplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/audioplay/synopsis",    { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/audioplay/text",        { { kAudioplayTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/audioplay/statistics",  { { kAudioplayStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/stageplay", { { kStageplayInformationViewMime, u8"\U000f02fd" },
                                                       { kStageplayParametersViewMime, u8"\U000f0493" } } },
        { "application/x-starc/document/stageplay/title-page",  { { kStageplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/stageplay/synopsis",    { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/stageplay/text",        { { kStageplayTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/stageplay/statistics",  { { kStageplayStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/characters",  { { "application/x-starc/editor/characters/relations", u8"\U000F0D3D" } } },
        { "application/x-starc/document/character",  { { "application/x-starc/editor/character/information", u8"\U000f02fd" } } },
        //
        { "application/x-starc/document/locations",  { { "application/x-starc/editor/locations/map", u8"\U000F0982" } } },
        { "application/x-starc/document/location",   { { "application/x-starc/editor/location/information", u8"\U000f02fd" } } },
        //
        { "application/x-starc/document/folder",     { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/text",       { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        //,
        { "application/x-starc/document/recycle-bin",       { { "application/x-starc/editor/recycle-bin", u8"\U000f01b4" } } }
      };

/**
 * @brief Карта соответсвий майм-типов навигаторов/редакторов к названиям библиотек с плагинами
 * @note Нужно это для того, чтобы не прогружать каждый раз плагин со всеми зависимостями лишь для
 *       извлечения майм-типа навигатора/редактора, а подгружать точечно только то, что нужно
 */
const QHash<QString, QString> kMimeToPlugin
    = { { "application/x-starc/editor/project/information", "*projectinformationplugin*" },
        //
        { kSimpleTextEditorMime, "*simpletextplugin*" },
        { kSimpleTextNavigatorMime, "*simpletextstructureplugin*" },
        //
        { "application/x-starc/editor/screenplay/information", "*screenplayinformationplugin*" },
        { "application/x-starc/editor/screenplay/parameters", "*screenplayparametersplugin*" },
        { kScreenplayTitlePageEditorMime, "*titlepageplugin*" },
        { kScreenplayTreatmentEditorMime, "*screenplaytreatmentplugin*" },
        { kScreenplayTreatmentNavigatorMime, "*screenplaytreatmentstructureplugin*" },
        { "application/x-starc/editor/screenplay/treatment-cards", "*screenplaytreatmentcardsplugin*" },
        { kScreenplayTextEditorMime, "*screenplaytextplugin*" },
        { kScreenplayTextNavigatorMime, "*screenplaytextstructureplugin*" },
        { "application/x-starc/editor/screenplay/cards", "*screenplaycardsplugin*" },
        { kScreenplayStatisticsViewMime, "*screenplaystatisticsplugin*" },
        { kScreenplayStatisticsNavigatorMime, "*screenplaystatisticsstructureplugin*" },
        //
        { "application/x-starc/editor/comicbook/information", "*comicbookinformationplugin*" },
        { "application/x-starc/editor/comicbook/parameters", "*comicbookparametersplugin*" },
        { kComicBookTitlePageEditorMime, "*titlepageplugin*" },
        { kComicBookTextEditorMime, "*comicbooktextplugin*" },
        { kComicBookTextNavigatorMime, "*comicbooktextstructureplugin*" },
        { kComicBookStatisticsViewMime, "*comicbookstatisticsplugin*" },
        //
        { kAudioplayInformationViewMime, "*audioplayinformationplugin*" },
        { kAudioplayParametersViewMime, "*audioplayparametersplugin*" },
        { kAudioplayTitlePageEditorMime, "*titlepageplugin*" },
        { kAudioplayTextEditorMime, "*audioplaytextplugin*" },
        { kAudioplayTextNavigatorMime, "*audioplaytextstructureplugin*" },
        { kAudioplayStatisticsViewMime, "*audioplaystatisticsplugin*" },
        //
        { kStageplayInformationViewMime, "*stageplayinformationplugin*" },
        { kStageplayParametersViewMime, "*stageplayparametersplugin*" },
        { kStageplayTitlePageEditorMime, "*titlepageplugin*" },
        { kStageplayTextEditorMime, "*stageplaytextplugin*" },
        { kStageplayTextNavigatorMime, "*stageplaytextstructureplugin*" },
        { kStageplayStatisticsViewMime, "*stageplaystatisticsplugin*" },
        //
        { "application/x-starc/editor/characters/relations", "*charactersrelationsplugin*" },
        { "application/x-starc/editor/character/information", "*characterinformationplugin*" },
        //
        { "application/x-starc/editor/locations/map", "*locationsmapplugin*" },
        { "application/x-starc/editor/location/information", "*locationinformationplugin*" },
        //
        { "application/x-starc/editor/recycle-bin", "*recyclebinplugin*" },
      };
// clang-format on

} // namespace

class PluginsBuilder::Implementation
{
public:
    /**
     * @brief Активировать плагин заданного типа указанной моделью
     */
    Ui::IDocumentView* activatePlugin(const QString& _mimeType,
                                      BusinessLayer::AbstractModel* _model, ViewType _type);

    /**
     * @brief Загруженные плагины <mime, plugin>
     */
    QHash<QString, ManagementLayer::IDocumentManager*> plugins;

    /**
     * @brief Текущий режим работы редакторов
     */
    DocumentEditingMode editingMode = DocumentEditingMode::Edit;
};

Ui::IDocumentView* PluginsBuilder::Implementation::activatePlugin(
    const QString& _mimeType, BusinessLayer::AbstractModel* _model, ViewType _type)
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
#elif defined(Q_OS_LINUX)
            ".so";
#elif defined(Q_OS_MAC)
            ".dylib";
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
        plugin->setEditingMode(editingMode);
        plugins.insert(_mimeType, plugin);
    }

    auto plugin = plugins.value(_mimeType);
    if (plugin == nullptr) {
        return nullptr;
    }

    Ui::IDocumentView* view = nullptr;
    switch (_type) {
    case Primary: {
        view = plugin->view(_model);
        break;
    }

    case Secondary: {
        view = plugin->secondaryView(_model);
        break;
    }

    default: {
        view = plugin->createView(_model);
        break;
    }
    }
    view->setEditingMode(editingMode);
    return view;
}


// ****


PluginsBuilder::PluginsBuilder()
    : d(new Implementation)
{
}

PluginsBuilder::~PluginsBuilder() = default;

IDocumentManager* PluginsBuilder::plugin(const QString& _mimeType) const
{
    auto plugin = d->plugins.find(_mimeType);
    if (plugin == d->plugins.end()) {
        return nullptr;
    }

    return plugin.value();
}

QVector<PluginsBuilder::EditorInfo> PluginsBuilder::editorsInfoFor(
    const QString& _documentMimeType) const
{
    return kDocumentToEditors.value(_documentMimeType);
}

QString PluginsBuilder::editorDescription(const QString& _documentMimeType,
                                          const QString& _editorMimeType) const
{
    // clang-format off
    const QHash<QString, QHash<QString, QString>> descriptions
        = { { "application/x-starc/document/project",
              { { "application/x-starc/editor/project/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about project") },
                { "application/x-starc/editor/project/collaborators",
                  QApplication::translate("ProjectPluginsBuilder", "Project collaborators") } } },
            //
            { "application/x-starc/document/screenplay",
              { { "application/x-starc/editor/screenplay/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about screenplay") },
                { "application/x-starc/editor/screenplay/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Screenplay parameters") } } },
            { "application/x-starc/document/screenplay/title-page",
              { { kScreenplayTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/screenplay/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/screenplay/treatment",
              { { kScreenplayTreatmentEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Treatment text") },
                { "application/x-starc/editor/screenplay/beatboard",
                  QApplication::translate("ProjectPluginsBuilder", "Beats") },
                { "application/x-starc/editor/screenplay/treatmentcards",
                  QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/screenplay/text",
              { { kScreenplayTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Screenplay text") },
                { "application/x-starc/editor/screenplay/cards",
                  QApplication::translate("ProjectPluginsBuilder", "Cards") },
                { "application/x-starc/editor/screenplay/timeline",
                  QApplication::translate("ProjectPluginsBuilder", "Timeline") } } },
            { "application/x-starc/document/screenplay/statistics",
              { { kScreenplayStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/comicbook",
              { { "application/x-starc/editor/comicbook/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about comic book") },
                { "application/x-starc/editor/comicbook/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Comic book parameters") } } },
            { "application/x-starc/document/comicbook/title-page",
              { { kComicBookTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/comicbook/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/comicbook/text",
              { { kComicBookTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Comic book text") },
                { "application/x-starc/editor/comicbook/cards",
                    QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/comicbook/statistics",
              { { kComicBookStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/audioplay",
              { { kAudioplayInformationViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Information about audioplay") },
                { "application/x-starc/editor/audioplay/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Audioplay parameters") } } },
            { "application/x-starc/document/audioplay/title-page",
              { { kAudioplayTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/audioplay/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/audioplay/text",
              { { kAudioplayTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Audioplay text") },
                { "application/x-starc/editor/audioplay/cards",
                    QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/audioplay/statistics",
              { { kAudioplayStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/stageplay",
              { { kStageplayInformationViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Information about stageplay") },
                { "application/x-starc/editor/stageplay/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Stageplay parameters") } } },
            { "application/x-starc/document/stageplay/title-page",
              { { kStageplayTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/stageplay/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/stageplay/text",
              { { kStageplayTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Stageplay text") },
                { "application/x-starc/editor/stageplay/cards",
                    QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/stageplay/statistics",
              { { kStageplayStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/characters",
              { { "application/x-starc/editor/characters/relations",
                  QApplication::translate("ProjectPluginsBuilder", "Characters relations") } } },
            { "application/x-starc/document/character",
              { { "application/x-starc/editor/character/information",
                  QApplication::translate("ProjectPluginsBuilder", "Character information") } } },
            //
            { "application/x-starc/document/locations",
              { { "application/x-starc/editor/locations/map",
                  QApplication::translate("ProjectPluginsBuilder", "Locations map") } } },
            { "application/x-starc/document/location",
              { { "application/x-starc/editor/location/information",
                  QApplication::translate("ProjectPluginsBuilder", "Location information") } } },
            //
            { "application/x-starc/document/folder",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Folder text") } } },
            { "application/x-starc/document/text",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Document text") } } },
            //,
            { "application/x-starc/document/recycle-bin",
              { { "application/x-starc/editor/recycle-bin",
                  QApplication::translate("ProjectPluginsBuilder", "Recycle bin summary info") } } }
             };
    // clang-format on
    return descriptions.value(_documentMimeType).value(_editorMimeType);
}

QString PluginsBuilder::navigatorMimeTypeFor(const QString& _editorMimeType) const
{
    return kEditorToNavigator.value(_editorMimeType);
}

Ui::IDocumentView* PluginsBuilder::activateView(const QString& _viewMimeType,
                                                BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(_viewMimeType, _model, ViewType::Primary);
}

Ui::IDocumentView* PluginsBuilder::activateSecondView(const QString& _viewMimeType,
                                                      BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(_viewMimeType, _model, ViewType::Secondary);
}

Ui::IDocumentView* PluginsBuilder::activateWindowView(const QString& _viewMimeType,
                                                      BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(_viewMimeType, _model, ViewType::Window);
}

void PluginsBuilder::bind(const QString& _viewMimeType, const QString& _navigatorMimeType) const
{
    auto viewPlugin = d->plugins.value(_viewMimeType);
    Q_ASSERT(viewPlugin);

    auto navigatorPlugin = d->plugins.value(_navigatorMimeType);
    Q_ASSERT(navigatorPlugin);

    viewPlugin->bind(navigatorPlugin);
    navigatorPlugin->bind(viewPlugin);
}

void PluginsBuilder::toggleViewFullScreen(bool _isFullScreen, const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->view();
    if (view != nullptr) {
        view->toggleFullScreen(_isFullScreen);
    }
}

void PluginsBuilder::toggleSecondaryViewFullScreen(bool _isFullScreen,
                                                   const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->secondaryView();
    if (view != nullptr) {
        view->toggleFullScreen(_isFullScreen);
    }
}

void PluginsBuilder::reconfigureAll() const
{
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->reconfigure({});
    }
}

void PluginsBuilder::reconfigurePlugin(const QString& _mimeType,
                                       const QStringList& _changedSettingsKeys) const
{
    auto plugin = this->plugin(_mimeType);
    if (!plugin) {
        return;
    }

    plugin->reconfigure(_changedSettingsKeys);
}

void PluginsBuilder::reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kSimpleTextEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kComicBookTitlePageEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureSimpleTextNavigator() const
{
    reconfigurePlugin(kSimpleTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kScreenplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTreatmentEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureScreenplayNavigator() const
{
    reconfigurePlugin(kScreenplayTreatmentNavigatorMime, {});
    reconfigurePlugin(kScreenplayTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureComicBookEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kComicBookTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kComicBookTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureComicBookNavigator() const
{
    reconfigurePlugin(kComicBookTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureAudioplayEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kAudioplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kAudioplayTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureAudioplayNavigator() const
{
    reconfigurePlugin(kAudioplayTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureStageplayEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kStageplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kStageplayTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureStageplayNavigator() const
{
    reconfigurePlugin(kStageplayTextNavigatorMime, {});
}

void PluginsBuilder::checkAvailabilityToEdit() const
{
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->checkAvailabilityToEdit();
    }
}

void PluginsBuilder::setEditingMode(DocumentEditingMode _mode) const
{
    if (d->editingMode == _mode) {
        return;
    }

    d->editingMode = _mode;
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->setEditingMode(d->editingMode);
    }
}

void PluginsBuilder::resetModels() const
{
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->saveSettings();
        plugin->resetModels();
    }
}

} // namespace ManagementLayer
