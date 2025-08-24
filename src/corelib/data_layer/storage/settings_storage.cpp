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
     * @brief Возможно ли сохранять параметры
     */
    bool isReadOnly = false;

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
    defaultValues.insert(kApplicationDensityKey, 0);
    defaultValues.insert(kApplicationUseAutoSaveKey, true);
    defaultValues.insert(kApplicationSaveBackupsKey, true);
    defaultValues.insert(kApplicationBackupsFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                             + "/starc/backups");
    defaultValues.insert(kApplicationBackupsQtyKey, 7);
    defaultValues.insert(kApplicationShowDocumentsPagesKey, true);
    defaultValues.insert(kApplicationUseTypewriterSoundKey, false);
    defaultValues.insert(kApplicationUseSpellCheckerKey, false);
    defaultValues.insert(kApplicationHighlightCurrentLineKey, false);
    defaultValues.insert(kApplicationFocusCurrentParagraphKey, false);
    defaultValues.insert(kApplicationUseTypewriterScrollingKey, false);
    defaultValues.insert(kApplicationCorrectDoubleCapitalsKey, true);
    defaultValues.insert(kApplicationCapitalizeSingleILetterKey, true);
    defaultValues.insert(kApplicationReplaceThreeDotsWithEllipsisKey, true);
    defaultValues.insert(kApplicationSmartQuotesKey, false);
    defaultValues.insert(kApplicationReplaceTwoDashesWithEmDashKey, false);
    defaultValues.insert(kApplicationAvoidMultipleSpacesKey, false);
    defaultValues.insert(kApplicationShortcutsImportKey, "Alt+I");
    defaultValues.insert(kApplicationShortcutsCurrentDocumentExportKey, "Alt+E");

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
        auto addSimpleTextEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsSimpleTextEditorKey, _actionType,
                                                toString(_from), _actionKey),
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
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::ChapterHeading1,
                                           TextParagraphType::ChapterHeading2);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::ChapterHeading1,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::ChapterHeading2,
                                           TextParagraphType::ChapterHeading3);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::ChapterHeading2,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::ChapterHeading3,
                                           TextParagraphType::ChapterHeading4);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::ChapterHeading3,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::ChapterHeading4,
                                           TextParagraphType::ChapterHeading5);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::ChapterHeading4,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::ChapterHeading5,
                                           TextParagraphType::ChapterHeading6);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::ChapterHeading5,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::ChapterHeading6,
                                           TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::ChapterHeading6,
                                             TextParagraphType::Text);
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
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::ChapterHeading1,
                                             TextParagraphType::ChapterHeading2);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::ChapterHeading1,
                                               TextParagraphType::ChapterHeading1);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::ChapterHeading2,
                                             TextParagraphType::ChapterHeading3);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::ChapterHeading2,
                                               TextParagraphType::ChapterHeading1);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::ChapterHeading3,
                                             TextParagraphType::ChapterHeading4);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::ChapterHeading3,
                                               TextParagraphType::ChapterHeading2);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::ChapterHeading4,
                                             TextParagraphType::ChapterHeading5);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::ChapterHeading4,
                                               TextParagraphType::ChapterHeading3);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::ChapterHeading5,
                                             TextParagraphType::ChapterHeading6);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::ChapterHeading5,
                                               TextParagraphType::ChapterHeading4);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::ChapterHeading6,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::ChapterHeading6,
                                               TextParagraphType::ChapterHeading5);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Text,
                                             TextParagraphType::InlineNote);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Text,
                                               TextParagraphType::ChapterHeading6);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                               TextParagraphType::InlineNote);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsSimpleTextEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading1, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading2, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading3, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading4, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading5, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading6, "Ctrl+6");
        addShortcut(BusinessLayer::TextParagraphType::Text, "Ctrl+7");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Shift+0");
        //
        defaultValues.insert(kComponentsSimpleTextAvailableKey, true);
        //
        defaultValues.insert(kComponentsSimpleTextEditorDefaultTemplateKey, "mono_cp_a4");
        defaultValues.insert(kComponentsSimpleTextEditorCorrectTextOnPageBreaksKey, false);
        //
        // Параметры навигатора простого текстового документа
        //
        defaultValues.insert(kComponentsSimpleTextNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsSimpleTextNavigatorSceneTextLinesKey, 1);
    }
    //
    // Параметры редактора тритмента сценария
    //
    {
        auto addTreatmentEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsScreenplayTreatmentEditorKey,
                                                _actionType, toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addTreatmentEditorStylesActionByTab
            = [addTreatmentEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addTreatmentEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addTreatmentEditorStylesActionByEnter
            = [addTreatmentEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addTreatmentEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addTreatmentEditorStylesJumpByTab =
            [addTreatmentEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addTreatmentEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addTreatmentEditorStylesJumpByEnter
            = [addTreatmentEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addTreatmentEditorStylesActionByEnter("jumping", _from, _to);
              };
        addTreatmentEditorStylesJumpByTab(TextParagraphType::SceneHeading,
                                          TextParagraphType::SceneCharacters);
        addTreatmentEditorStylesJumpByEnter(TextParagraphType::SceneHeading,
                                            TextParagraphType::BeatHeading);
        addTreatmentEditorStylesJumpByTab(TextParagraphType::SceneCharacters,
                                          TextParagraphType::BeatHeading);
        addTreatmentEditorStylesJumpByEnter(TextParagraphType::SceneCharacters,
                                            TextParagraphType::BeatHeading);
        addTreatmentEditorStylesJumpByTab(TextParagraphType::BeatHeading,
                                          TextParagraphType::BeatHeading);
        addTreatmentEditorStylesJumpByEnter(TextParagraphType::BeatHeading,
                                            TextParagraphType::BeatHeading);
        addTreatmentEditorStylesJumpByTab(TextParagraphType::SequenceHeading,
                                          TextParagraphType::SceneHeading);
        addTreatmentEditorStylesJumpByEnter(TextParagraphType::SequenceHeading,
                                            TextParagraphType::SceneHeading);
        addTreatmentEditorStylesJumpByTab(TextParagraphType::ActHeading,
                                          TextParagraphType::SequenceHeading);
        addTreatmentEditorStylesJumpByEnter(TextParagraphType::ActHeading,
                                            TextParagraphType::SceneHeading);
        //
        auto addTreatmentEditorStylesChangeByTab =
            [addTreatmentEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addTreatmentEditorStylesActionByTab("changing", _from, _to);
            };
        auto addTreatmentEditorStylesChangeByEnter
            = [addTreatmentEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addTreatmentEditorStylesActionByEnter("changing", _from, _to);
              };
        addTreatmentEditorStylesChangeByTab(TextParagraphType::SceneHeading,
                                            TextParagraphType::BeatHeading);
        addTreatmentEditorStylesChangeByEnter(TextParagraphType::SceneHeading,
                                              TextParagraphType::SceneHeading);
        addTreatmentEditorStylesChangeByTab(TextParagraphType::SceneCharacters,
                                            TextParagraphType::BeatHeading);
        addTreatmentEditorStylesChangeByEnter(TextParagraphType::SceneCharacters,
                                              TextParagraphType::SceneCharacters);
        addTreatmentEditorStylesChangeByTab(TextParagraphType::BeatHeading,
                                            TextParagraphType::BeatHeading);
        addTreatmentEditorStylesChangeByEnter(TextParagraphType::BeatHeading,
                                              TextParagraphType::SceneHeading);
        addTreatmentEditorStylesChangeByTab(TextParagraphType::SequenceHeading,
                                            TextParagraphType::SequenceHeading);
        addTreatmentEditorStylesChangeByEnter(TextParagraphType::SequenceHeading,
                                              TextParagraphType::SequenceHeading);
        addTreatmentEditorStylesChangeByTab(TextParagraphType::ActHeading,
                                            TextParagraphType::ActHeading);
        addTreatmentEditorStylesChangeByEnter(TextParagraphType::ActHeading,
                                              TextParagraphType::ActHeading);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(QString("%1/shortcuts/%2")
                                           .arg(kComponentsScreenplayTreatmentEditorKey,
                                                BusinessLayer::toString(_type)),
                                       QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::BeatHeading, "Ctrl+Shift+1");
        addShortcut(BusinessLayer::TextParagraphType::SceneCharacters, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::SequenceHeading, "Ctrl+Space");
        addShortcut(BusinessLayer::TextParagraphType::ActHeading, "Ctrl+Shift+Space");
    }
    //
    // Параметры редактора сценария
    //
    {
        auto addScreenplayEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsScreenplayEditorKey, _actionType,
                                                toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addScreenplayEditorStylesActionByTab
            = [addScreenplayEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                                TextParagraphType _to) {
                  addScreenplayEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addScreenplayEditorStylesActionByEnter
            = [addScreenplayEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                                TextParagraphType _to) {
                  addScreenplayEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addScreenplayEditorStylesJumpByTab =
            [addScreenplayEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addScreenplayEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addScreenplayEditorStylesJumpByEnter
            = [addScreenplayEditorStylesActionByEnter](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addScreenplayEditorStylesActionByEnter("jumping", _from, _to);
              };
        addScreenplayEditorStylesJumpByTab(TextParagraphType::UnformattedText,
                                           TextParagraphType::UnformattedText);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::UnformattedText,
                                             TextParagraphType::UnformattedText);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::SceneHeading,
                                           TextParagraphType::SceneCharacters);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::SceneHeading,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::SceneCharacters,
                                           TextParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::SceneCharacters,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::BeatHeading,
                                           TextParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::BeatHeading,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Action, TextParagraphType::Character);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Action, TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Character,
                                           TextParagraphType::Parenthetical);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Character,
                                             TextParagraphType::Dialogue);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Parenthetical,
                                           TextParagraphType::Dialogue);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Parenthetical,
                                             TextParagraphType::Dialogue);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Dialogue,
                                           TextParagraphType::Parenthetical);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Dialogue,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Lyrics,
                                           TextParagraphType::Parenthetical);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Lyrics, TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Transition,
                                           TextParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Transition,
                                             TextParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::Shot, TextParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::Shot, TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::InlineNote,
                                           TextParagraphType::Action);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::InlineNote,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::SequenceHeading,
                                           TextParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::SequenceHeading,
                                             TextParagraphType::SceneHeading);
        addScreenplayEditorStylesJumpByTab(TextParagraphType::ActHeading,
                                           TextParagraphType::SequenceHeading);
        addScreenplayEditorStylesJumpByEnter(TextParagraphType::ActHeading,
                                             TextParagraphType::SceneHeading);
        //
        auto addScreenplayEditorStylesChangeByTab =
            [addScreenplayEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addScreenplayEditorStylesActionByTab("changing", _from, _to);
            };
        auto addScreenplayEditorStylesChangeByEnter
            = [addScreenplayEditorStylesActionByEnter](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addScreenplayEditorStylesActionByEnter("changing", _from, _to);
              };
        addScreenplayEditorStylesChangeByTab(TextParagraphType::UnformattedText,
                                             TextParagraphType::UnformattedText);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::UnformattedText,
                                               TextParagraphType::UnformattedText);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::SceneHeading,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::SceneHeading,
                                               TextParagraphType::SceneHeading);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::SceneCharacters,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::SceneCharacters,
                                               TextParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::BeatHeading,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::BeatHeading,
                                               TextParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Action,
                                             TextParagraphType::Character);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Action,
                                               TextParagraphType::SceneHeading);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Character,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Character,
                                               TextParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Parenthetical,
                                             TextParagraphType::Dialogue);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Parenthetical,
                                               TextParagraphType::Parenthetical);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Dialogue,
                                             TextParagraphType::Parenthetical);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Dialogue,
                                               TextParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Lyrics,
                                             TextParagraphType::Parenthetical);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Lyrics,
                                               TextParagraphType::Action);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Transition,
                                             TextParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Transition,
                                               TextParagraphType::Transition);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::Shot, TextParagraphType::Action);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::Shot, TextParagraphType::Shot);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                             TextParagraphType::InlineNote);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                               TextParagraphType::InlineNote);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::SequenceHeading,
                                             TextParagraphType::SceneHeading);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::SequenceHeading,
                                               TextParagraphType::ActHeading);
        addScreenplayEditorStylesChangeByTab(TextParagraphType::ActHeading,
                                             TextParagraphType::SequenceHeading);
        addScreenplayEditorStylesChangeByEnter(TextParagraphType::ActHeading,
                                               TextParagraphType::ActHeading);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsScreenplayEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::TextParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::BeatHeading, "Ctrl+Shift+1");
        addShortcut(BusinessLayer::TextParagraphType::SceneCharacters, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::Action, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::Character, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::Parenthetical, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::Dialogue, "Ctrl+6");
        addShortcut(BusinessLayer::TextParagraphType::Lyrics, "Ctrl+7");
        addShortcut(BusinessLayer::TextParagraphType::Shot, "Ctrl+8");
        addShortcut(BusinessLayer::TextParagraphType::Transition, "Ctrl+9");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Shift+0");
        addShortcut(BusinessLayer::TextParagraphType::SequenceHeading, "Ctrl+Space");
        addShortcut(BusinessLayer::TextParagraphType::ActHeading, "Ctrl+Shift+Space");
        //
        defaultValues.insert(kComponentsScreenplayAvailableKey, true);
        //
        defaultValues.insert(kComponentsScreenplayEditorDefaultTemplateKey, "world_cp");
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumbersKey, false);
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumbersOnLeftKey, true);
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumbersOnRightKey, true);
        defaultValues.insert(kComponentsScreenplayEditorContinueDialogueKey, false);
        defaultValues.insert(kComponentsScreenplayEditorCorrectTextOnPageBreaksKey, true);
        defaultValues.insert(kComponentsScreenplayEditorSaveItemsFromTextKey, true);
        defaultValues.insert(kComponentsScreenplayEditorShowHintsForAllItemsKey, true);
        defaultValues.insert(kComponentsScreenplayEditorShowHintsForPrimaryItemsKey, false);
        defaultValues.insert(kComponentsScreenplayEditorShowHintsForSecondaryItemsKey, false);
        defaultValues.insert(kComponentsScreenplayEditorShowHintsForTertiaryItemsKey, false);
        defaultValues.insert(kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey,
                             true);
        defaultValues.insert(kComponentsScreenplayEditorUseOpenBracketInDialogueForParentheticalKey,
                             true);
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsScreenplayNavigatorShowBeatsKey, true);
        defaultValues.insert(kComponentsScreenplayNavigatorShowBeatsInTreatmentKey, true);
        defaultValues.insert(kComponentsScreenplayNavigatorShowBeatsInScreenplayKey, false);
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
        defaultValues.insert(
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey, 1.0);
        defaultValues.insert(kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey,
                             1.5);
        defaultValues.insert(
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey, 2.0);
        defaultValues.insert(
            kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey, 2.4);
        defaultValues.insert(
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey, 2.0);
        defaultValues.insert(
            kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey, 0.0);
        //
        // Параметры карточек сценария
        //
        defaultValues.insert(kComponentsScreenplayCardsLockCardsKey, true);
        defaultValues.insert(kComponentsScreenplayCardsArrangeByRowsKey, true);
        defaultValues.insert(kComponentsScreenplayCardsCardsSizeKey, 10);
        defaultValues.insert(kComponentsScreenplayCardsCardsRatioKey, 2);
        defaultValues.insert(kComponentsScreenplayCardsCardsSpacingKey, 20);
        defaultValues.insert(kComponentsScreenplayCardsCardsInRowKey, -1);
        //
        // Параметры таймлайна сценария
        //
        defaultValues.insert(kComponentsScreenplayTimelineArrangeHorizontalKey, true);
        defaultValues.insert(kComponentsScreenplayTimelineCardsSizeKey, 10);
        defaultValues.insert(kComponentsScreenplayTimelineCardsRatioKey, 2);
        defaultValues.insert(kComponentsScreenplayTimelineCardsSpacingKey, 20);
    }
    //
    // Параметры редактора комикса
    //
    {
        auto addComicBookEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsComicBookEditorKey, _actionType,
                                                toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addComicBookEditorStylesActionByTab
            = [addComicBookEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addComicBookEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addComicBookEditorStylesActionByEnter
            = [addComicBookEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addComicBookEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addComicBookEditorStylesJumpByTab =
            [addComicBookEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addComicBookEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addComicBookEditorStylesJumpByEnter
            = [addComicBookEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addComicBookEditorStylesActionByEnter("jumping", _from, _to);
              };
        addComicBookEditorStylesJumpByTab(TextParagraphType::UnformattedText,
                                          TextParagraphType::UnformattedText);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::UnformattedText,
                                            TextParagraphType::UnformattedText);
        addComicBookEditorStylesJumpByTab(TextParagraphType::PageHeading,
                                          TextParagraphType::PanelHeading);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::PageHeading,
                                            TextParagraphType::PanelHeading);
        addComicBookEditorStylesJumpByTab(TextParagraphType::PanelHeading,
                                          TextParagraphType::Character);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::PanelHeading,
                                            TextParagraphType::Description);
        addComicBookEditorStylesJumpByTab(TextParagraphType::Description,
                                          TextParagraphType::Character);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::Description,
                                            TextParagraphType::Description);
        addComicBookEditorStylesJumpByTab(TextParagraphType::Character,
                                          TextParagraphType::Parenthetical);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::Character,
                                            TextParagraphType::Dialogue);
        addComicBookEditorStylesJumpByTab(TextParagraphType::Parenthetical,
                                          TextParagraphType::Dialogue);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::Parenthetical,
                                            TextParagraphType::Dialogue);
        addComicBookEditorStylesJumpByTab(TextParagraphType::Dialogue,
                                          TextParagraphType::Parenthetical);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::Dialogue,
                                            TextParagraphType::Description);
        addComicBookEditorStylesJumpByTab(TextParagraphType::InlineNote,
                                          TextParagraphType::Description);
        addComicBookEditorStylesJumpByEnter(TextParagraphType::InlineNote,
                                            TextParagraphType::Description);
        //
        auto addComicBookEditorStylesChangeByTab =
            [addComicBookEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addComicBookEditorStylesActionByTab("changing", _from, _to);
            };
        auto addComicBookEditorStylesChangeByEnter
            = [addComicBookEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addComicBookEditorStylesActionByEnter("changing", _from, _to);
              };
        addComicBookEditorStylesChangeByTab(TextParagraphType::UnformattedText,
                                            TextParagraphType::UnformattedText);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::UnformattedText,
                                              TextParagraphType::UnformattedText);
        addComicBookEditorStylesChangeByTab(TextParagraphType::PageHeading,
                                            TextParagraphType::PanelHeading);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::PageHeading,
                                              TextParagraphType::PageHeading);
        addComicBookEditorStylesChangeByTab(TextParagraphType::PanelHeading,
                                            TextParagraphType::Description);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::PanelHeading,
                                              TextParagraphType::PageHeading);
        addComicBookEditorStylesChangeByTab(TextParagraphType::Description,
                                            TextParagraphType::Character);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::Description,
                                              TextParagraphType::PanelHeading);
        addComicBookEditorStylesChangeByTab(TextParagraphType::Character,
                                            TextParagraphType::Description);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::Character,
                                              TextParagraphType::Description);
        addComicBookEditorStylesChangeByTab(TextParagraphType::Parenthetical,
                                            TextParagraphType::Dialogue);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::Parenthetical,
                                              TextParagraphType::Parenthetical);
        addComicBookEditorStylesChangeByTab(TextParagraphType::Dialogue,
                                            TextParagraphType::Parenthetical);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::Dialogue,
                                              TextParagraphType::Description);
        addComicBookEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                            TextParagraphType::InlineNote);
        addComicBookEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                              TextParagraphType::InlineNote);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsComicBookEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::TextParagraphType::PageHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::PanelHeading, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::Description, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::Character, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::Parenthetical, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::Dialogue, "Ctrl+6");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Shift+0");
        //
        defaultValues.insert(kComponentsComicBookAvailableKey, true);
        //
        defaultValues.insert(kComponentsComicBookEditorDefaultTemplateKey, "world");
        defaultValues.insert(kComponentsComicBookEditorShowDialogueNumberKey, true);
        defaultValues.insert(kComponentsComicBookEditorSaveItemsFromTextKey, true);
        defaultValues.insert(kComponentsComicBookEditorShowHintsForAllItemsKey, true);
        defaultValues.insert(kComponentsComicBookEditorShowHintsForPrimaryItemsKey, false);
        defaultValues.insert(kComponentsComicBookEditorShowHintsForSecondaryItemsKey, false);
        defaultValues.insert(kComponentsComicBookEditorShowHintsForTertiaryItemsKey, false);
        defaultValues.insert(kComponentsComicBookEditorShowCharacterSuggestionsInEmptyBlockKey,
                             false);
        defaultValues.insert(kComponentsComicBookEditorUseOpenBracketInDialogueForParentheticalKey,
                             true);
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsComicBookNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsComicBookNavigatorSceneTextLinesKey, 1);
    }
    //
    // Параметры редактора аудиопостановки
    //
    {
        auto addAudioplayEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsAudioplayEditorKey, _actionType,
                                                toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addAudioplayEditorStylesActionByTab
            = [addAudioplayEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addAudioplayEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addAudioplayEditorStylesActionByEnter
            = [addAudioplayEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addAudioplayEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addAudioplayEditorStylesJumpByTab =
            [addAudioplayEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addAudioplayEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addAudioplayEditorStylesJumpByEnter
            = [addAudioplayEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addAudioplayEditorStylesActionByEnter("jumping", _from, _to);
              };
        addAudioplayEditorStylesJumpByTab(TextParagraphType::UnformattedText,
                                          TextParagraphType::UnformattedText);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::UnformattedText,
                                            TextParagraphType::UnformattedText);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::SceneHeading, TextParagraphType::Cue);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::SceneHeading,
                                            TextParagraphType::Character);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::Character,
                                          TextParagraphType::Dialogue);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::Character,
                                            TextParagraphType::Dialogue);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::Dialogue, TextParagraphType::Cue);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::Dialogue,
                                            TextParagraphType::Character);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::Sound, TextParagraphType::Music);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::Sound, TextParagraphType::Character);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::Music, TextParagraphType::Sound);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::Music, TextParagraphType::Character);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::Cue, TextParagraphType::SceneHeading);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::Cue, TextParagraphType::Character);
        addAudioplayEditorStylesJumpByTab(TextParagraphType::InlineNote,
                                          TextParagraphType::Character);
        addAudioplayEditorStylesJumpByEnter(TextParagraphType::InlineNote,
                                            TextParagraphType::Character);
        //
        auto addAudioplayEditorStylesChangeByTab =
            [addAudioplayEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addAudioplayEditorStylesActionByTab("changing", _from, _to);
            };
        auto addAudioplayEditorStylesChangeByEnter
            = [addAudioplayEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addAudioplayEditorStylesActionByEnter("changing", _from, _to);
              };
        addAudioplayEditorStylesChangeByTab(TextParagraphType::UnformattedText,
                                            TextParagraphType::Character);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::UnformattedText,
                                              TextParagraphType::Character);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::SceneHeading,
                                            TextParagraphType::Character);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::SceneHeading,
                                              TextParagraphType::Character);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::Character, TextParagraphType::Sound);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::Character,
                                              TextParagraphType::SceneHeading);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::Dialogue, TextParagraphType::Sound);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::Dialogue,
                                              TextParagraphType::SceneHeading);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::Sound, TextParagraphType::Music);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::Sound,
                                              TextParagraphType::SceneHeading);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::Music, TextParagraphType::Cue);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::Music,
                                              TextParagraphType::SceneHeading);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::Cue, TextParagraphType::Character);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::Cue,
                                              TextParagraphType::SceneHeading);
        addAudioplayEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                            TextParagraphType::Character);
        addAudioplayEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                              TextParagraphType::Character);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsAudioplayEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::TextParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::Character, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::Dialogue, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::Sound, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::Music, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::Cue, "Ctrl+6");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Shift+0");
        //
        defaultValues.insert(kComponentsAudioplayAvailableKey, true);
        //
        defaultValues.insert(kComponentsAudioplayEditorDefaultTemplateKey, "bbc_scene");
        defaultValues.insert(kComponentsAudioplayEditorShowBlockNumbersKey, false);
        defaultValues.insert(kComponentsAudioplayEditorContinueBlockNumbersKey, true);
        defaultValues.insert(kComponentsAudioplayEditorSaveItemsFromTextKey, true);
        defaultValues.insert(kComponentsAudioplayEditorShowHintsForAllItemsKey, true);
        defaultValues.insert(kComponentsAudioplayEditorShowHintsForPrimaryItemsKey, false);
        defaultValues.insert(kComponentsAudioplayEditorShowHintsForSecondaryItemsKey, false);
        defaultValues.insert(kComponentsAudioplayEditorShowHintsForTertiaryItemsKey, false);
        defaultValues.insert(kComponentsAudioplayEditorShowCharacterSuggestionsInEmptyBlockKey,
                             false);
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsAudioplayNavigatorShowSceneNumberKey, true);
        defaultValues.insert(kComponentsAudioplayNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsAudioplayNavigatorSceneTextLinesKey, 1);
        //
        // Параметры хронометража сценария
        //
        defaultValues.insert(kComponentsAudioplayDurationByWordsWordsKey, 140);
        defaultValues.insert(kComponentsAudioplayDurationByWordsDurationKey, 60);
    }
    //
    // Параметры редактора пьес
    //
    {
        auto addStageplayEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsStageplayEditorKey, _actionType,
                                                toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addStageplayEditorStylesActionByTab
            = [addStageplayEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addStageplayEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addStageplayEditorStylesActionByEnter
            = [addStageplayEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                               TextParagraphType _to) {
                  addStageplayEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addStageplayEditorStylesJumpByTab =
            [addStageplayEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addStageplayEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addStageplayEditorStylesJumpByEnter
            = [addStageplayEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addStageplayEditorStylesActionByEnter("jumping", _from, _to);
              };
        addStageplayEditorStylesJumpByTab(TextParagraphType::UnformattedText,
                                          TextParagraphType::UnformattedText);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::UnformattedText,
                                            TextParagraphType::UnformattedText);
        addStageplayEditorStylesJumpByTab(TextParagraphType::SceneHeading,
                                          TextParagraphType::Action);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::SceneHeading,
                                            TextParagraphType::Character);
        addStageplayEditorStylesJumpByTab(TextParagraphType::Character,
                                          TextParagraphType::Parenthetical);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::Character,
                                            TextParagraphType::Dialogue);
        addStageplayEditorStylesJumpByTab(TextParagraphType::Parenthetical,
                                          TextParagraphType::Dialogue);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::Parenthetical,
                                            TextParagraphType::Dialogue);
        addStageplayEditorStylesJumpByTab(TextParagraphType::Dialogue,
                                          TextParagraphType::Parenthetical);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::Dialogue,
                                            TextParagraphType::Character);
        addStageplayEditorStylesJumpByTab(TextParagraphType::Action,
                                          TextParagraphType::SceneHeading);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::Action,
                                            TextParagraphType::Character);
        addStageplayEditorStylesJumpByTab(TextParagraphType::InlineNote,
                                          TextParagraphType::Character);
        addStageplayEditorStylesJumpByEnter(TextParagraphType::InlineNote,
                                            TextParagraphType::Character);
        //
        auto addStageplayEditorStylesChangeByTab =
            [addStageplayEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addStageplayEditorStylesActionByTab("changing", _from, _to);
            };
        auto addStageplayEditorStylesChangeByEnter
            = [addStageplayEditorStylesActionByEnter](TextParagraphType _from,
                                                      TextParagraphType _to) {
                  addStageplayEditorStylesActionByEnter("changing", _from, _to);
              };
        addStageplayEditorStylesChangeByTab(TextParagraphType::UnformattedText,
                                            TextParagraphType::Character);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::UnformattedText,
                                              TextParagraphType::Character);
        addStageplayEditorStylesChangeByTab(TextParagraphType::SceneHeading,
                                            TextParagraphType::Character);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::SceneHeading,
                                              TextParagraphType::Character);
        addStageplayEditorStylesChangeByTab(TextParagraphType::Character,
                                            TextParagraphType::Action);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::Character,
                                              TextParagraphType::SceneHeading);
        addStageplayEditorStylesChangeByTab(TextParagraphType::Parenthetical,
                                            TextParagraphType::Dialogue);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::Parenthetical,
                                              TextParagraphType::Dialogue);
        addStageplayEditorStylesChangeByTab(TextParagraphType::Dialogue, TextParagraphType::Action);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::Dialogue,
                                              TextParagraphType::SceneHeading);
        addStageplayEditorStylesChangeByTab(TextParagraphType::Action,
                                            TextParagraphType::Character);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::Action,
                                              TextParagraphType::Character);
        addStageplayEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                            TextParagraphType::Character);
        addStageplayEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                              TextParagraphType::Character);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsStageplayEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::TextParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::Character, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::Parenthetical, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::Dialogue, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::Action, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Shift+0");
        //
        defaultValues.insert(kComponentsStageplayAvailableKey, true);
        //
        defaultValues.insert(kComponentsStageplayEditorDefaultTemplateKey, "bbc");
        defaultValues.insert(kComponentsStageplayEditorSaveItemsFromTextKey, true);
        defaultValues.insert(kComponentsStageplayEditorShowHintsForAllItemsKey, true);
        defaultValues.insert(kComponentsStageplayEditorShowHintsForPrimaryItemsKey, false);
        defaultValues.insert(kComponentsStageplayEditorShowHintsForSecondaryItemsKey, false);
        defaultValues.insert(kComponentsStageplayEditorShowHintsForTertiaryItemsKey, false);
        defaultValues.insert(kComponentsStageplayEditorShowCharacterSuggestionsInEmptyBlockKey,
                             false);
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsStageplayNavigatorShowSceneNumberKey, true);
        defaultValues.insert(kComponentsStageplayNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsStageplayNavigatorSceneTextLinesKey, 1);
    }
    //
    // Параметры редактора плана романа
    //
    {
        auto addNovelOutlineEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(QString("%1/styles-%2/from-%3-by-%4")
                                           .arg(kComponentsNovelOutlineEditorKey, _actionType,
                                                toString(_from), _actionKey),
                                       toString(_to));
              };
        auto addNovelOutlineEditorStylesActionByTab
            = [addNovelOutlineEditorStylesAction](const QString& _actionType,
                                                  TextParagraphType _from, TextParagraphType _to) {
                  addNovelOutlineEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addNovelOutlineEditorStylesActionByEnter
            = [addNovelOutlineEditorStylesAction](const QString& _actionType,
                                                  TextParagraphType _from, TextParagraphType _to) {
                  addNovelOutlineEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addNovelOutlineEditorStylesJumpByTab
            = [addNovelOutlineEditorStylesActionByTab](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addNovelOutlineEditorStylesActionByTab("jumping", _from, _to);
              };
        auto addNovelOutlineEditorStylesJumpByEnter
            = [addNovelOutlineEditorStylesActionByEnter](TextParagraphType _from,
                                                         TextParagraphType _to) {
                  addNovelOutlineEditorStylesActionByEnter("jumping", _from, _to);
              };
        addNovelOutlineEditorStylesJumpByTab(TextParagraphType::SceneHeading,
                                             TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesJumpByEnter(TextParagraphType::SceneHeading,
                                               TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesJumpByTab(TextParagraphType::BeatHeading,
                                             TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesJumpByEnter(TextParagraphType::BeatHeading,
                                               TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesJumpByTab(TextParagraphType::ChapterHeading,
                                             TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesJumpByEnter(TextParagraphType::ChapterHeading,
                                               TextParagraphType::SceneHeading);
        addNovelOutlineEditorStylesJumpByTab(TextParagraphType::PartHeading,
                                             TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesJumpByEnter(TextParagraphType::PartHeading,
                                               TextParagraphType::ChapterHeading);
        //
        auto addNovelOutlineEditorStylesChangeByTab
            = [addNovelOutlineEditorStylesActionByTab](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addNovelOutlineEditorStylesActionByTab("changing", _from, _to);
              };
        auto addNovelOutlineEditorStylesChangeByEnter
            = [addNovelOutlineEditorStylesActionByEnter](TextParagraphType _from,
                                                         TextParagraphType _to) {
                  addNovelOutlineEditorStylesActionByEnter("changing", _from, _to);
              };
        addNovelOutlineEditorStylesChangeByTab(TextParagraphType::SceneHeading,
                                               TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesChangeByEnter(TextParagraphType::SceneHeading,
                                                 TextParagraphType::ChapterHeading);
        addNovelOutlineEditorStylesChangeByTab(TextParagraphType::BeatHeading,
                                               TextParagraphType::BeatHeading);
        addNovelOutlineEditorStylesChangeByEnter(TextParagraphType::BeatHeading,
                                                 TextParagraphType::SceneHeading);
        addNovelOutlineEditorStylesChangeByTab(TextParagraphType::ChapterHeading,
                                               TextParagraphType::SceneHeading);
        addNovelOutlineEditorStylesChangeByEnter(TextParagraphType::ChapterHeading,
                                                 TextParagraphType::PartHeading);
        addNovelOutlineEditorStylesChangeByTab(TextParagraphType::PartHeading,
                                               TextParagraphType::ChapterHeading);
        addNovelOutlineEditorStylesChangeByEnter(TextParagraphType::PartHeading,
                                                 TextParagraphType::PartHeading);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsNovelOutlineEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::BeatHeading, "Ctrl+Shift+1");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading, "Ctrl+Space");
        addShortcut(BusinessLayer::TextParagraphType::PartHeading, "Ctrl+Shift+Space");
    }
    //
    // Параметры редактора романа
    //
    {
        auto addNovelEditorStylesAction
            = [this](const QString& _actionType, const QString& _actionKey, TextParagraphType _from,
                     TextParagraphType _to) {
                  defaultValues.insert(
                      QString("%1/styles-%2/from-%3-by-%4")
                          .arg(kComponentsNovelEditorKey, _actionType, toString(_from), _actionKey),
                      toString(_to));
              };
        auto addNovelEditorStylesActionByTab
            = [addNovelEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                           TextParagraphType _to) {
                  addNovelEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addNovelEditorStylesActionByEnter
            = [addNovelEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                           TextParagraphType _to) {
                  addNovelEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addNovelEditorStylesJumpByTab
            = [addNovelEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                  addNovelEditorStylesActionByTab("jumping", _from, _to);
              };
        auto addNovelEditorStylesJumpByEnter
            = [addNovelEditorStylesActionByEnter](TextParagraphType _from, TextParagraphType _to) {
                  addNovelEditorStylesActionByEnter("jumping", _from, _to);
              };
        addNovelEditorStylesJumpByTab(TextParagraphType::UnformattedText,
                                      TextParagraphType::UnformattedText);
        addNovelEditorStylesJumpByEnter(TextParagraphType::UnformattedText,
                                        TextParagraphType::UnformattedText);
        addNovelEditorStylesJumpByTab(TextParagraphType::SceneHeading, TextParagraphType::Text);
        addNovelEditorStylesJumpByEnter(TextParagraphType::SceneHeading, TextParagraphType::Text);
        addNovelEditorStylesJumpByTab(TextParagraphType::BeatHeading, TextParagraphType::Text);
        addNovelEditorStylesJumpByEnter(TextParagraphType::BeatHeading, TextParagraphType::Text);
        addNovelEditorStylesJumpByTab(TextParagraphType::Text, TextParagraphType::Text);
        addNovelEditorStylesJumpByEnter(TextParagraphType::Text, TextParagraphType::Text);
        addNovelEditorStylesJumpByTab(TextParagraphType::InlineNote, TextParagraphType::Text);
        addNovelEditorStylesJumpByEnter(TextParagraphType::InlineNote, TextParagraphType::Text);
        addNovelEditorStylesJumpByTab(TextParagraphType::ChapterHeading,
                                      TextParagraphType::SceneHeading);
        addNovelEditorStylesJumpByEnter(TextParagraphType::ChapterHeading,
                                        TextParagraphType::SceneHeading);
        addNovelEditorStylesJumpByTab(TextParagraphType::PartHeading,
                                      TextParagraphType::ChapterHeading);
        addNovelEditorStylesJumpByEnter(TextParagraphType::PartHeading,
                                        TextParagraphType::SceneHeading);
        //
        auto addNovelEditorStylesChangeByTab
            = [addNovelEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                  addNovelEditorStylesActionByTab("changing", _from, _to);
              };
        auto addNovelEditorStylesChangeByEnter
            = [addNovelEditorStylesActionByEnter](TextParagraphType _from, TextParagraphType _to) {
                  addNovelEditorStylesActionByEnter("changing", _from, _to);
              };
        addNovelEditorStylesChangeByTab(TextParagraphType::UnformattedText,
                                        TextParagraphType::UnformattedText);
        addNovelEditorStylesChangeByEnter(TextParagraphType::UnformattedText,
                                          TextParagraphType::UnformattedText);
        addNovelEditorStylesChangeByTab(TextParagraphType::SceneHeading, TextParagraphType::Text);
        addNovelEditorStylesChangeByEnter(TextParagraphType::SceneHeading,
                                          TextParagraphType::ChapterHeading);
        addNovelEditorStylesChangeByTab(TextParagraphType::BeatHeading, TextParagraphType::Text);
        addNovelEditorStylesChangeByEnter(TextParagraphType::BeatHeading, TextParagraphType::Text);
        addNovelEditorStylesChangeByTab(TextParagraphType::Text, TextParagraphType::Text);
        addNovelEditorStylesChangeByEnter(TextParagraphType::Text, TextParagraphType::SceneHeading);
        addNovelEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                        TextParagraphType::InlineNote);
        addNovelEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                          TextParagraphType::InlineNote);
        addNovelEditorStylesChangeByTab(TextParagraphType::ChapterHeading,
                                        TextParagraphType::SceneHeading);
        addNovelEditorStylesChangeByEnter(TextParagraphType::ChapterHeading,
                                          TextParagraphType::PartHeading);
        addNovelEditorStylesChangeByTab(TextParagraphType::PartHeading,
                                        TextParagraphType::ChapterHeading);
        addNovelEditorStylesChangeByEnter(TextParagraphType::PartHeading,
                                          TextParagraphType::PartHeading);
        //
        auto addShortcut
            = [this](BusinessLayer::TextParagraphType _type, const QString& _shortcut) {
                  defaultValues.insert(
                      QString("%1/shortcuts/%2")
                          .arg(kComponentsNovelEditorKey, BusinessLayer::toString(_type)),
                      QKeySequence(_shortcut).toString(QKeySequence::NativeText));
              };
        addShortcut(BusinessLayer::TextParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::TextParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::BeatHeading, "Ctrl+Shift+1");
        addShortcut(BusinessLayer::TextParagraphType::Text, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Shift+0");
        addShortcut(BusinessLayer::TextParagraphType::ChapterHeading, "Ctrl+Space");
        addShortcut(BusinessLayer::TextParagraphType::PartHeading, "Ctrl+Shift+Space");
        //
        defaultValues.insert(kComponentsNovelAvailableKey, true);
        //
        defaultValues.insert(kComponentsNovelEditorDefaultTemplateKey, "manuscript_t_a4");
        defaultValues.insert(kComponentsNovelEditorCorrectTextOnPageBreaksKey, false);
        //
        // Параметры навигатора
        //
        defaultValues.insert(kComponentsNovelNavigatorShowBeatsKey, true);
        defaultValues.insert(kComponentsNovelNavigatorShowBeatsInOutlineKey, true);
        defaultValues.insert(kComponentsNovelNavigatorShowBeatsInTextKey, false);
        defaultValues.insert(kComponentsNovelNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsNovelNavigatorSceneTextLinesKey, 1);
        defaultValues.insert(kComponentsNovelNavigatorCounterTypeKey, 0);
        //
        // Параметры карточек сценария
        //
        defaultValues.insert(kComponentsNovelCardsLockCardsKey, true);
        defaultValues.insert(kComponentsNovelCardsArrangeByRowsKey, true);
        defaultValues.insert(kComponentsNovelCardsCardsSizeKey, 10);
        defaultValues.insert(kComponentsNovelCardsCardsRatioKey, 2);
        defaultValues.insert(kComponentsNovelCardsCardsSpacingKey, 20);
        defaultValues.insert(kComponentsNovelCardsCardsInRowKey, -1);
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
    return cachedValues.value(_key);
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
    if (d->isReadOnly) {
        return;
    }

    //
    // Кэшируем значение
    //
    d->cacheValue(_key, _value, _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == SettingsPlace::Application) {
        d->appSettings.setValue(_key.toUtf8().toHex(), _value);
    } else {
        MapperFacade::settingsMapper()->setValue(_key, _value.toString());
    }
}

void SettingsStorage::setValues(const QString& _valuesGroup, const QVariantMap& _values,
                                SettingsStorage::SettingsPlace _settingsPlace)
{
    if (d->isReadOnly) {
        return;
    }

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

void SettingsStorage::sync(SettingsPlace _settingsPlace)
{
    if (_settingsPlace == SettingsPlace::Application) {
        d->appSettings.sync();
    }
}

QString SettingsStorage::accountName() const
{
    auto name = value(kAccountUserNameKey, SettingsPlace::Application).toString();
    if (name.isEmpty()) {
        name = value(kSystemUsernameKey, SettingsPlace::Application).toString();
    }
    return name;
}

QString SettingsStorage::accountEmail() const
{
    return value(kAccountEmailKey, SettingsPlace::Application).toString();
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

void SettingsStorage::resetToDefaults()
{
    //
    // Запрещаем писать настройки до перезагрузки приложения
    //
    d->isReadOnly = true;

    //
    // Стираем всё, что было изменено, кроме нескольких параметров, которые по-сути не являются
    // настройками приложения и пользователь не ожидает, что они будут удалены при сбросе параметров
    //
    QHash<QString, QVariant> settings;
    for (const auto& key : d->appSettings.allKeys()) {
        const QString realKey = QByteArray::fromHex(key.toUtf8());
        if (realKey == kDeviceUuidKey || realKey == kApplicationProjectsKey
            || realKey.startsWith(kAccountGroupKey) || realKey.startsWith(kProjectGroupKey)
            || realKey.startsWith(QLatin1String("starc"))) {
            settings.insert(key, d->appSettings.value(key));
        }
    }
    d->appSettings.clear();

    //
    // Восстанавливаем необходимые параметры
    //
    for (auto iter = settings.begin(); iter != settings.end(); ++iter) {
        d->appSettings.setValue(iter.key(), iter.value());
    }
    //
    // Форсим сохранение файла с настройками
    //
    d->appSettings.sync();
}

SettingsStorage::SettingsStorage()
    : d(new Implementation)
{
}

} // namespace DataStorageLayer
