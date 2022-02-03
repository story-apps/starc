#include "settings_storage.h"

#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/simple_text_template.h>
#include <data_layer/mapper/mapper_facade.h>
#include <data_layer/mapper/settings_mapper.h>
#include <ui/design_system/design_system.h>

#include <QDir>
#include <QKeySequence>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>

using DataMappingLayer::MapperFacade;


namespace DataStorageLayer {

namespace {
/**
 * @brief Имя пользователя из системы
 */
static QString systemUserName()
{
    QString name = qgetenv("USER");
    if (name.isEmpty()) {
        //
        // Windows
        //
        name = QString::fromLocal8Bit(qgetenv("USERNAME"));
        if (name.isEmpty()) {
            name = "user";
        }
    }
    return name;
}
} // namespace

class SettingsStorage::Implementation
{
public:
    Implementation();

    /**
     * @brief Загрузить параметр из кэша
     */
    QVariant cachedValue(const QString& _key, SettingsPlace _settingsPlace, bool& _ok) const;

    /**
     * @brief Сохранить параметр в кэше
     */
    void cacheValue(const QString& _key, const QVariant& _value, SettingsPlace _settingsPlace);


    /**
     * @brief Настройки приложения
     */
    QSettings appSettings;

    /**
     * @brief Значения параметров по умолчанию
     */
    QVariantMap defaultValues;

    /**
     * @brief Кэшированные значения параметров
     */
    /** @{ */
    QVariantMap cachedValuesApp;
    QVariantMap cachedValuesDb;
    /** @} */
};

SettingsStorage::Implementation::Implementation()
{
    using namespace BusinessLayer;

    //
    // Настроим значения параметров по умолчанию
    //

    //
    // Для приложения
    //
    defaultValues.insert(kApplicationConfiguredKey, false);
    defaultValues.insert(kApplicationLanguagedKey, QLocale::AnyLanguage);
    defaultValues.insert(kApplicationThemeKey, static_cast<int>(Ui::ApplicationTheme::Light));
    defaultValues.insert(
        kApplicationCustomThemeColorsKey,
        "323740ffffff2ab177f8f8f2272b34f8f8f222262ef8f8f2ec3740f8f8f2000000f8f8f2272b34f8f8f2");
    defaultValues.insert(kApplicationScaleFactorKey, 1.0);
    defaultValues.insert(kApplicationUseAutoSaveKey, true);
    defaultValues.insert(kApplicationSaveBackupsKey, true);
    defaultValues.insert(kApplicationBackupsFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                             + "/starc/backups");
    defaultValues.insert(kApplicationShowDocumentsPagesKey, true);
    defaultValues.insert(kApplicationUseTypewriterSoundKey, false);
    defaultValues.insert(kApplicationUseSpellCheckerKey, false);
    defaultValues.insert(kApplicationHighlightCurrentLineKey, false);
    defaultValues.insert(kApplicationFocusCurrentParagraphKey, false);
    defaultValues.insert(kApplicationUseTypewriterScrollingKey, false);
    defaultValues.insert(kProjectTypeKey, 0);
    defaultValues.insert(kProjectSaveFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                             + "/starc/projects");
    defaultValues.insert(kProjectImportFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    defaultValues.insert(kProjectExportFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    //
    // Параметры редактора текста
    //
    {
        const QString kSimpleTextEditorKey = "simple-text/editor";
        auto addSimpleTextEditorStylesAction
            = [this, kSimpleTextEditorKey](const QString& _actionType, const QString& _actionKey,
                                           TextParagraphType _from, TextParagraphType _to) {
                  defaultValues.insert(
                      QString("%1/styles-%2/from-%3-by-%4")
                          .arg(kSimpleTextEditorKey, _actionType, toString(_from), _actionKey),
                      toString(_to));
              };
        auto addSimpleTextEditorStylesActionByTab
            = [addSimpleTextEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                                TextParagraphType _to) {
                  addSimpleTextEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addSimpleTextEditorStylesActionByEnter
            = [addSimpleTextEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                                TextParagraphType _to) {
                  addSimpleTextEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addSimpleTextEditorStylesJumpByTab =
            [addSimpleTextEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addSimpleTextEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addSimpleTextEditorStylesJumpByEnter
            = [addSimpleTextEditorStylesActionByEnter](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addSimpleTextEditorStylesActionByEnter("jumping", _from, _to);
              };
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading1,
                                           TextParagraphType::Heading2);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading1, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading2,
                                           TextParagraphType::Heading3);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading2, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading3,
                                           TextParagraphType::Heading4);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading3, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading4,
                                           TextParagraphType::Heading5);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading4, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading5,
                                           TextParagraphType::Heading6);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading5, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading6, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading6, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Text, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Text, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::InlineNote, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::InlineNote,
                                             TextParagraphType::Text);
        //
        auto addSimpleTextEditorStylesChangeByTab =
            [addSimpleTextEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addSimpleTextEditorStylesActionByTab("changing", _from, _to);
            };
        auto addSimpleTextEditorStylesChangeByEnter
            = [addSimpleTextEditorStylesActionByEnter](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addSimpleTextEditorStylesActionByEnter("changing", _from, _to);
              };
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading1,
                                             TextParagraphType::Heading2);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading1,
                                               TextParagraphType::Heading1);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading2,
                                             TextParagraphType::Heading3);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading2,
                                               TextParagraphType::Heading1);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading3,
                                             TextParagraphType::Heading4);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading3,
                                               TextParagraphType::Heading2);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading4,
                                             TextParagraphType::Heading5);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading4,
                                               TextParagraphType::Heading3);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading5,
                                             TextParagraphType::Heading6);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading5,
                                               TextParagraphType::Heading4);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading6, TextParagraphType::Text);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading6,
                                               TextParagraphType::Heading5);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Text,
                                             TextParagraphType::InlineNote);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Text,
                                               TextParagraphType::Heading6);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                               TextParagraphType::InlineNote);
        //
        auto addShortcut = [this, kSimpleTextEditorKey](BusinessLayer::TextParagraphType _type,
                                                        const QString& _shortcut) {
            defaultValues.insert(QString("%1/shortcuts/%2")
                                     .arg(kSimpleTextEditorKey, BusinessLayer::toString(_type)),
                                 QKeySequence(_shortcut).toString(QKeySequence::NativeText));
        };
        addShortcut(BusinessLayer::TextParagraphType::Heading1, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::Heading2, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::Heading3, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::Heading4, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::Heading5, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::Heading6, "Ctrl+6");
        addShortcut(BusinessLayer::TextParagraphType::Text, "Ctrl+7");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Esc");
        //
        defaultValues.insert(kComponentsSimpleTextEditorDefaultTemplateKey, "mono_cp_a4");
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsSimpleTextNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsSimpleTextNavigatorSceneTextLinesKey, 1);
    }
    //
    // Параметры редактора сценария
    //
    {
        auto addScreenplayEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey,
                     ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsScreenplayEditorKey, _actionType,
                                                toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addScreenplayEditorStylesActionByTab
            = [addScreenplayEditorStylesAction](const QString& _actionType,
                                                ScreenplayParagraphType _from,
                                                ScreenplayParagraphType _to) {
                  addScreenplayEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addScreenplayEditorStylesActionByEnter
            = [addScreenplayEditorStylesAction](const QString& _actionType,
                                                ScreenplayParagraphType _from,
                                                ScreenplayParagraphType _to) {
                  addScreenplayEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addScreenplayEditorStylesJumpByTab
            = [addScreenplayEditorStylesActionByTab](ScreenplayParagraphType _from,
                                                     ScreenplayParagraphType _to) {
                  addScreenplayEditorStylesActionByTab("jumping", _from, _to);
              };
        auto addScreenplayEditorStylesJumpByEnter
            = [addScreenplayEditorStylesActionByEnter](ScreenplayParagraphType _from,
                                                       ScreenplayParagraphType _to) {
                  addScreenplayEditorStylesActionByEnter("jumping", _from, _to);
              };
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::UnformattedText,
                                           ScreenplayParagraphType::UnformattedText);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::UnformattedText,
                                             ScreenplayParagraphType::UnformattedText);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::SceneHeading,
                                           ScreenplayParagraphType::SceneCharacters);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::SceneHeading,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::SceneCharacters,
                                           ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::SceneCharacters,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Action,
                                           ScreenplayParagraphType::Character);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Action,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Character,
                                           ScreenplayParagraphType::Parenthetical);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Character,
                                             ScreenplayParagraphType::Dialogue);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Parenthetical,
                                           ScreenplayParagraphType::Dialogue);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Parenthetical,
                                             ScreenplayParagraphType::Dialogue);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Dialogue,
                                           ScreenplayParagraphType::Parenthetical);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Dialogue,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Lyrics,
                                           ScreenplayParagraphType::Parenthetical);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Lyrics,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Transition,
                                           ScreenplayParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Transition,
                                             ScreenplayParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Shot,
                                           ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Shot,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::InlineNote,
                                           ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::InlineNote,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::FolderHeader,
                                           ScreenplayParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::FolderHeader,
                                             ScreenplayParagraphType::SceneHeading);
        //
        auto addScreenplayEditorStylesChangeByTab
            = [addScreenplayEditorStylesActionByTab](ScreenplayParagraphType _from,
                                                     ScreenplayParagraphType _to) {
                  addScreenplayEditorStylesActionByTab("changing", _from, _to);
              };
        auto addScreenplayEditorStylesChangeByEnter
            = [addScreenplayEditorStylesActionByEnter](ScreenplayParagraphType _from,
                                                       ScreenplayParagraphType _to) {
                  addScreenplayEditorStylesActionByEnter("changing", _from, _to);
              };
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::UnformattedText,
                                             ScreenplayParagraphType::UnformattedText);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::UnformattedText,
                                               ScreenplayParagraphType::UnformattedText);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::SceneHeading,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::SceneHeading,
                                               ScreenplayParagraphType::SceneHeading);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::SceneCharacters,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::SceneCharacters,
                                               ScreenplayParagraphType::SceneCharacters);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Action,
                                             ScreenplayParagraphType::Character);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Action,
                                               ScreenplayParagraphType::SceneHeading);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Character,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Character,
                                               ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Parenthetical,
                                             ScreenplayParagraphType::Dialogue);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Parenthetical,
                                               ScreenplayParagraphType::Parenthetical);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Dialogue,
                                             ScreenplayParagraphType::Parenthetical);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Dialogue,
                                               ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Lyrics,
                                             ScreenplayParagraphType::Parenthetical);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Lyrics,
                                               ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Transition,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Transition,
                                               ScreenplayParagraphType::Transition);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Shot,
                                             ScreenplayParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Shot,
                                               ScreenplayParagraphType::Shot);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::InlineNote,
                                             ScreenplayParagraphType::InlineNote);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::InlineNote,
                                               ScreenplayParagraphType::InlineNote);
        addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::FolderHeader,
                                             ScreenplayParagraphType::FolderHeader);
        addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::FolderHeader,
                                               ScreenplayParagraphType::FolderHeader);
        //
        auto addShortcut
            = [this](BusinessLayer::ScreenplayParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsScreenplayEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::ScreenplayParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::ScreenplayParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::ScreenplayParagraphType::SceneCharacters, "Ctrl+2");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Action, "Ctrl+3");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Character, "Ctrl+4");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Parenthetical, "Ctrl+5");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Dialogue, "Ctrl+6");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Lyrics, "Ctrl+7");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Shot, "Ctrl+8");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Transition, "Ctrl+9");
        addShortcut(BusinessLayer::ScreenplayParagraphType::InlineNote, "Ctrl+Esc");
        addShortcut(BusinessLayer::ScreenplayParagraphType::FolderHeader, "Ctrl+Space");
        //
        defaultValues.insert(kComponentsScreenplayEditorDefaultTemplateKey, "world_cp");
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumbersKey, false);
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumbersOnLeftKey, true);
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumbersOnRightKey, true);
        defaultValues.insert(kComponentsScreenplayEditorContinueDialogueKey, true);
        defaultValues.insert(kComponentsScreenplayEditorUseCharactersFromTextKey, false);
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsScreenplayNavigatorShowSceneNumberKey, true);
        defaultValues.insert(kComponentsScreenplayNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsScreenplayNavigatorSceneTextLinesKey, 1);
        //
        // Параметры хронометража сценария
        //
        defaultValues.insert(kComponentsScreenplayDurationTypeKey, 0);
        defaultValues.insert(kComponentsScreenplayDurationByPageDurationKey, 60);
        defaultValues.insert(kComponentsScreenplayDurationByCharactersCharactersKey, 1350);
        defaultValues.insert(kComponentsScreenplayDurationByCharactersIncludeSpacesKey, true);
        defaultValues.insert(kComponentsScreenplayDurationByCharactersDurationKey, 60);
    }
    //
    // Параметры редактора комикса
    //
    {
        const QString kComicBookEditorKey = "comicbook-editor";
        auto addComicBookEditorStylesAction =
            [this, kComicBookEditorKey](const QString& _actionType, const QString& _actionKey,
                                        ComicBookParagraphType _from, ComicBookParagraphType _to) {
                defaultValues.insert(
                    QString("%1/styles-%2/from-%3-by-%4")
                        .arg(kComicBookEditorKey, _actionType, toString(_from), _actionKey),
                    toString(_to));
            };
        auto addComicBookEditorStylesActionByTab
            = [addComicBookEditorStylesAction](const QString& _actionType,
                                               ComicBookParagraphType _from,
                                               ComicBookParagraphType _to) {
                  addComicBookEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addComicBookEditorStylesActionByEnter
            = [addComicBookEditorStylesAction](const QString& _actionType,
                                               ComicBookParagraphType _from,
                                               ComicBookParagraphType _to) {
                  addComicBookEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addComicBookEditorStylesJumpByTab
            = [addComicBookEditorStylesActionByTab](ComicBookParagraphType _from,
                                                    ComicBookParagraphType _to) {
                  addComicBookEditorStylesActionByTab("jumping", _from, _to);
              };
        auto addComicBookEditorStylesJumpByEnter
            = [addComicBookEditorStylesActionByEnter](ComicBookParagraphType _from,
                                                      ComicBookParagraphType _to) {
                  addComicBookEditorStylesActionByEnter("jumping", _from, _to);
              };
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::UnformattedText,
                                          ComicBookParagraphType::UnformattedText);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::UnformattedText,
                                            ComicBookParagraphType::UnformattedText);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::Page,
                                          ComicBookParagraphType::Panel);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::Page,
                                            ComicBookParagraphType::Panel);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::Panel,
                                          ComicBookParagraphType::Character);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::Panel,
                                            ComicBookParagraphType::Description);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::Description,
                                          ComicBookParagraphType::Character);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::Description,
                                            ComicBookParagraphType::Description);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::Character,
                                          ComicBookParagraphType::Dialogue);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::Character,
                                            ComicBookParagraphType::Dialogue);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::Dialogue,
                                          ComicBookParagraphType::Character);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::Dialogue,
                                            ComicBookParagraphType::Description);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::InlineNote,
                                          ComicBookParagraphType::Description);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::InlineNote,
                                            ComicBookParagraphType::Description);
        addComicBookEditorStylesJumpByTab(ComicBookParagraphType::FolderHeader,
                                          ComicBookParagraphType::Page);
        addComicBookEditorStylesJumpByEnter(ComicBookParagraphType::FolderHeader,
                                            ComicBookParagraphType::Page);
        //
        auto addComicBookEditorStylesChangeByTab
            = [addComicBookEditorStylesActionByTab](ComicBookParagraphType _from,
                                                    ComicBookParagraphType _to) {
                  addComicBookEditorStylesActionByTab("changing", _from, _to);
              };
        auto addComicBookEditorStylesChangeByEnter
            = [addComicBookEditorStylesActionByEnter](ComicBookParagraphType _from,
                                                      ComicBookParagraphType _to) {
                  addComicBookEditorStylesActionByEnter("changing", _from, _to);
              };
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::UnformattedText,
                                            ComicBookParagraphType::UnformattedText);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::UnformattedText,
                                              ComicBookParagraphType::UnformattedText);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::Page,
                                            ComicBookParagraphType::Panel);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::Page,
                                              ComicBookParagraphType::Page);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::Panel,
                                            ComicBookParagraphType::Description);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::Panel,
                                              ComicBookParagraphType::Page);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::Description,
                                            ComicBookParagraphType::Character);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::Description,
                                              ComicBookParagraphType::Panel);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::Character,
                                            ComicBookParagraphType::Description);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::Character,
                                              ComicBookParagraphType::Description);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::Dialogue,
                                            ComicBookParagraphType::Character);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::Dialogue,
                                              ComicBookParagraphType::Description);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::InlineNote,
                                            ComicBookParagraphType::InlineNote);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::InlineNote,
                                              ComicBookParagraphType::InlineNote);
        addComicBookEditorStylesChangeByTab(ComicBookParagraphType::FolderHeader,
                                            ComicBookParagraphType::FolderHeader);
        addComicBookEditorStylesChangeByEnter(ComicBookParagraphType::FolderHeader,
                                              ComicBookParagraphType::FolderHeader);
        //
        auto addShortcut = [this, kComicBookEditorKey](BusinessLayer::ComicBookParagraphType _type,
                                                       const QString& _shortcut) {
            defaultValues.insert(
                QString("%1/shortcuts/%2").arg(kComicBookEditorKey, BusinessLayer::toString(_type)),
                QKeySequence(_shortcut).toString(QKeySequence::NativeText));
        };
        addShortcut(BusinessLayer::ComicBookParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::ComicBookParagraphType::Page, "Ctrl+1");
        addShortcut(BusinessLayer::ComicBookParagraphType::Panel, "Ctrl+2");
        addShortcut(BusinessLayer::ComicBookParagraphType::Description, "Ctrl+3");
        addShortcut(BusinessLayer::ComicBookParagraphType::Character, "Ctrl+4");
        addShortcut(BusinessLayer::ComicBookParagraphType::Dialogue, "Ctrl+5");
        addShortcut(BusinessLayer::ComicBookParagraphType::InlineNote, "Ctrl+Esc");
        //
        defaultValues.insert(kComponentsComicBookEditorDefaultTemplateKey, "world");
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsComicBookNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsComicBookNavigatorSceneTextLinesKey, 1);
    }


    defaultValues.insert(kSystemUsernameKey, systemUserName());
}

QVariant SettingsStorage::Implementation::cachedValue(const QString& _key,
                                                      SettingsStorage::SettingsPlace _settingsPlace,
                                                      bool& _ok) const
{
    const QVariantMap& cachedValues
        = _settingsPlace == SettingsPlace::Application ? cachedValuesApp : cachedValuesDb;
    _ok = cachedValues.contains(_key);
    return cachedValuesApp.value(_key);
}

void SettingsStorage::Implementation::cacheValue(const QString& _key, const QVariant& _value,
                                                 SettingsStorage::SettingsPlace _settingsPlace)
{
    QVariantMap& cachedValues
        = _settingsPlace == SettingsPlace::Application ? cachedValuesApp : cachedValuesDb;
    cachedValues.insert(_key, _value);
}


// ****


SettingsStorage::~SettingsStorage() = default;

void SettingsStorage::setValue(const QString& _key, const QVariant& _value,
                               SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Кэшируем значение
    //
    d->cacheValue(_key, _value, _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == SettingsPlace::Application) {
        d->appSettings.setValue(_key.toUtf8().toHex(), _value);
        d->appSettings.sync();
    } else {
        MapperFacade::settingsMapper()->setValue(_key, _value.toString());
    }
}

void SettingsStorage::setValues(const QString& _valuesGroup, const QVariantMap& _values,
                                SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Кэшируем значение
    //
    d->cacheValue(_valuesGroup, _values, _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == SettingsPlace::Application) {
        //
        // Очистим группу
        //
        {
            d->appSettings.beginGroup(_valuesGroup);
            d->appSettings.remove("");
            d->appSettings.endGroup();
        }

        //
        // Откроем группу
        //
        d->appSettings.beginGroup(_valuesGroup);

        //
        // Сохраним значения
        //
        for (const QString& key : _values.keys()) {
            d->cacheValue(key, _values.value(key), _settingsPlace);
            d->appSettings.setValue(key.toUtf8().toHex(), _values.value(key));
        }

        //
        // Закроем группу
        //
        d->appSettings.endGroup();

        //
        // Сохраняем изменения в файл
        //
        d->appSettings.sync();
    }
    //
    // В базу данных карта параметров не умеет сохраняться
    //
    else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Database settings can't save group of settings");
    }
}

QVariant SettingsStorage::value(const QString& _key, SettingsStorage::SettingsPlace _settingsPlace,
                                const QVariant& _defaultValue) const
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QVariant value = d->cachedValue(_key, _settingsPlace, hasCachedValue);

    //
    // Если в кэше нашлось нужное значение, вернём его
    //
    if (hasCachedValue) {
        return value;
    }

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (_settingsPlace == SettingsPlace::Application) {
        value = d->appSettings.value(_key.toUtf8().toHex(), QVariant());
    } else {
        value = MapperFacade::settingsMapper()->value(_key);
    }

    //
    // Если удалось загрузить
    //
    if (!value.isNull()) {
        //
        // Сохраняем значение в кэш
        //
        d->cacheValue(_key, value, _settingsPlace);

        return value;
    }

    //
    // Если параметр не задан, то используем заданное значение по умолчанию
    //
    if (!_defaultValue.isNull()) {
        return _defaultValue;
    }

    //
    // В конце концов пробуем вытащить что-нибудь из предустановленных умолчальных значений
    //
    return d->defaultValues.value(_key);
}

QVariantMap SettingsStorage::values(const QString& _valuesGroup,
                                    SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QVariantMap values = d->cachedValue(_valuesGroup, _settingsPlace, hasCachedValue).toMap();

    if (hasCachedValue) {
        return values;
    }

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (_settingsPlace == SettingsPlace::Application) {
        //
        // Откроем группу для считывания
        //
        d->appSettings.beginGroup(_valuesGroup);

        //
        // Получим все ключи
        //
        QStringList keys = d->appSettings.childKeys();

        //
        // Получим все значения
        //
        for (const QString& key : keys) {
            values.insert(QByteArray::fromHex(key.toUtf8()), d->appSettings.value(key));
        }

        //
        // Закроем группу
        //
        d->appSettings.endGroup();
    }
    //
    // Из базы данных карта параметров не умеет загружаться
    //
    else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Database settings can't load group of settings");
    }

    //
    // Сохраняем значение в кэш
    //
    d->cacheValue(_valuesGroup, values, _settingsPlace);

    return values;
}

QString SettingsStorage::accountName() const
{
    //
    // TODO: получать имя пользователя если он авторизован
    //

    return value(kSystemUsernameKey, SettingsPlace::Application).toString();
}

QString SettingsStorage::accountEmail() const
{
    //
    // TODO
    //

    return {};
}

QString SettingsStorage::documentFolderPath(const QString& _key) const
{
    QString folderPath = value(_key, SettingsPlace::Application).toString();
    if (folderPath.isEmpty()) {
        folderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return folderPath;
}

QString SettingsStorage::documentFilePath(const QString& _key, const QString& _fileName) const
{
    const QString filePath = documentFolderPath(_key) + QDir::separator() + _fileName;
    return QDir::toNativeSeparators(filePath);
}

void SettingsStorage::setDocumentFolderPath(const QString& _key, const QString& _filePath)
{
    setValue(_key, QFileInfo(_filePath).absoluteDir().absolutePath(), SettingsPlace::Application);
}

SettingsStorage::SettingsStorage()
    : d(new Implementation)
{
}

} // namespace DataStorageLayer
