#pragma once

#include <ui/design_system/design_system.h>

#include <QLocale>
#include <QObject>

namespace Ui {
enum class ApplicationTheme;
class ThemeSetupView;
} // namespace Ui


namespace ManagementLayer {

class PluginsBuilder;

/**
 * @brief Менеджер экрана настроек
 */
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    SettingsManager(QObject* _parent, QWidget* _parentWidget,
                    const PluginsBuilder& _pluginsBuilder);
    ~SettingsManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Задать виджет настройки темы
     */
    void setThemeSetupView(Ui::ThemeSetupView* _view);

    /**
     * @brief Обновить используемый коэффициент масштабирования в представлении
     */
    void updateScaleFactor();

signals:
    /**
     * @brief Пользователь хочет закрыть настройки
     */
    void closeSettingsRequested();

    /**
     * @brief Изменились параметры приложения
     */
    void applicationLanguageChanged(QLocale::Language _language);
    void applicationLanguageFileChanged(const QString _filePath);
    void applicationUseSpellCheckerChanged(bool _use);
    void applicationSpellCheckerLanguageChanged(const QString& _languageCode);
    void applicationThemeChanged(Ui::ApplicationTheme _theme);
    void applicationCustomThemeColorsChanged(const Ui::DesignSystem::Color& _color);
    void applicationScaleFactorChanged(qreal _scaleFactor);
    void applicationDensityChanged(qreal _isCompact);
    void applicationUseAutoSaveChanged(bool _use);
    void applicationSaveBackupsChanged(bool _save);
    void applicationBackupsFolderChanged(const QString& _path);
    void applicationBackupsQtyChanged(int _qty);

    /**
     * @brief Изменились параметры компонентов
     */
    void simpleTextEditorChanged(const QStringList& _changedSettingsKeys);
    void simpleTextNavigatorChanged();
    void screenplayEditorChanged(const QStringList& _changedSettingsKeys);
    void screenplayNavigatorChanged();
    void screenplayDurationChanged();
    void comicBookEditorChanged(const QStringList& _changedSettingsKeys);
    void comicBookNavigatorChanged();
    void audioplayEditorChanged(const QStringList& _changedSettingsKeys);
    void audioplayNavigatorChanged();
    void audioplayDurationChanged();
    void stageplayEditorChanged(const QStringList& _changedSettingsKeys);
    void stageplayNavigatorChanged();
    void novelEditorChanged(const QStringList& _changedSettingsKeys);
    void novelNavigatorChanged();

    /**
     * @brief Пользователь хочет сбросить настройки к заводским
     */
    void resetToDefaultsRequested();

protected:
    /**
     * @brief Реализуем фильтр на событие смены языка и дизайн системы, чтобы обновить значения в
     * представлении
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    //
    // Сохранение параметров приложения
    //
    void setApplicationLanguage(int _language);
    void setApplicationLanguageFile(const QString& _filePath);
    void setApplicationTheme(Ui::ApplicationTheme _theme);
    void setApplicationCustomThemeColors(const Ui::DesignSystem::Color& _color);
    void setApplicationScaleFactor(qreal _scaleFactor);
    void setApplicationDensity(int _density);
    void setApplicationUseAutoSave(bool _use);
    void setApplicationSaveBackups(bool _save);
    void setApplicationBackupsFolder(const QString& _path);
    void setApplicationBackupsQty(int _qty);
    void setApplicationShowDocumentsPages(bool _show);
    void setApplicationUseTypeWriterSound(bool _use);
    void setApplicationUseSpellChecker(bool _use);
    void setApplicationSpellCheckerLanguage(const QString& _languageCode);
    void loadSpellingDictionary(const QString& _languageCode);
    void loadSpellingDictionaryAffFile(const QString& _languageCode);
    void loadSpellingDictionaryDicFile(const QString& _languageCode);
    void setApplicationHighlightCurrentLine(bool _highlight);
    void setApplicationFocusCurrentParagraph(bool _focus);
    void setApplicationUseTypewriterScrolling(bool _use);
    void setApplicationCapitalizeSingleILetter(bool _capitalize);
    void setApplicationReplaceThreeDotsWithEllipsis(bool _replace);
    void setApplicationUseSmartQuotes(bool _use);
    void setApplicationReplaceTwoDashesWithEmDash(bool _replace);
    void setApplicationAvoidMultipleSpaces(bool _avoid);

    //
    // Сохранение параметров редактора сценария
    //
    void setSimpleTextAvailable(bool _available);
    //
    void setSimpleTextEditorDefaultTemplate(const QString& _templateId);
    //
    void setSimpleTextNavigatorShowSceneText(bool _show, int _lines);

    //
    // Сохранение параметров редактора сценария
    //
    void setScreenplayAvailable(bool _available);
    //
    void setScreenplayEditorDefaultTemplate(const QString& _templateId);
    void setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight);
    void setScreenplayEditorShowDialogueNumber(bool _show);
    void setScreenplayEditorContinueDialogue(bool _continue);
    void setScreenplayEditorCorrectTextOnPageBreaks(bool _correct);
    void setScreenplayEditorSaveItemsFromText(bool _use);
    void setScreenplayEditorShowHints(bool _all, bool _primary, bool _secondary, bool _tertiary);
    void setScreenplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    void setScreenplayEditorUseOpenBracketInDialogueForParenthetical(bool _use);
    //
    void setScreenplayNavigatorShowBeats(bool _show, bool _inTreatment, bool _inScreenplay);
    void setScreenplayNavigatorShowSceneNumber(bool _show);
    void setScreenplayNavigatorShowSceneText(bool _show, int _lines);
    //
    void setScreenplayDurationType(int _type);
    void setScreenplayDurationByPageDuration(int _duration);
    void setScreenplayDurationByCharactersCharacters(int _characters);
    void setScreenplayDurationByCharactersIncludeSpaces(bool _include);
    void setScreenplayDurationByCharactersDuration(int _duration);
    void setScreenplayDurationConfigurablePerParagraphForAction(qreal _duration);
    void setScreenplayDurationConfigurablePerEvery50ForAction(qreal _duration);
    void setScreenplayDurationConfigurablePerParagraphForDialogue(qreal _duration);
    void setScreenplayDurationConfigurablePerEvery50ForDialogue(qreal _duration);
    void setScreenplayDurationConfigurablePerParagraphForSceneHeading(qreal _duration);
    void setScreenplayDurationConfigurablePerEvery50ForSceneHeading(qreal _duration);

    //
    // Сохранение параметров редактора комикса
    //
    void setComicBookAvailable(bool _available);
    //
    void setComicBookEditorDefaultTemplate(const QString& _templateId);
    void setComicBookEditorShowDialogueNumber(bool _show);
    void setComicBookEditorSaveItemsFromText(bool _use);
    void setComicBookEditorShowHints(bool _all, bool _primary, bool _secondary, bool _tertiary);
    void setComicBookEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setComicBookNavigatorShowSceneText(bool _show, int _lines);

    //
    // Сохранение параметров редактора подкастов
    //
    void setAudioplayAvailable(bool _available);
    //
    void setAudioplayEditorDefaultTemplate(const QString& _templateId);
    void setAudioplayEditorShowBlockNumber(bool _show, bool _continued);
    void setAudioplayEditorSaveItemsFromText(bool _use);
    void setAudioplayEditorShowHints(bool _all, bool _primary, bool _secondary, bool _tertiary);
    void setAudioplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setAudioplayNavigatorShowSceneNumber(bool _show);
    void setAudioplayNavigatorShowSceneText(bool _show, int _lines);
    //
    void setAudioplayDurationByWordsWords(int _words);
    void setAudioplayDurationByWordsDuration(int _duration);

    //
    // Сохранение параметров редактора пьес
    //
    void setStageplayAvailable(bool _available);
    //
    void setStageplayEditorDefaultTemplate(const QString& _templateId);
    void setStageplayEditorSaveItemsFromText(bool _use);
    void setStageplayEditorShowHints(bool _all, bool _primary, bool _secondary, bool _tertiary);
    void setStageplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setStageplayNavigatorShowSceneNumber(bool _show);
    void setStageplayNavigatorShowSceneText(bool _show, int _lines);

    //
    // Сохранение параметров редактора сценария
    //
    void setNovelAvailable(bool _available);
    //
    void setNovelEditorDefaultTemplate(const QString& _templateId);
    //
    void setNovelNavigatorShowSceneText(bool _show, int _lines);
    void setNovelNavigatorCounterType(int _type);

    //
    // Сохранение параметров горячих клавиш
    //
    void setShortcutsForSimpleTextEdit(const QString& _blockType, const QString& _shortcut,
                                       const QString& _jumpByTab, const QString& _jumpByEnter,
                                       const QString& _changeByTab, const QString& _changeByEnter);
    void setShortcutsForScreenplayEdit(const QString& _blockType, const QString& _shortcut,
                                       const QString& _jumpByTab, const QString& _jumpByEnter,
                                       const QString& _changeByTab, const QString& _changeByEnter);
    void setShortcutsForComicBookEdit(const QString& _blockType, const QString& _shortcut,
                                      const QString& _jumpByTab, const QString& _jumpByEnter,
                                      const QString& _changeByTab, const QString& _changeByEnter);
    void setShortcutsForAudioplayEdit(const QString& _blockType, const QString& _shortcut,
                                      const QString& _jumpByTab, const QString& _jumpByEnter,
                                      const QString& _changeByTab, const QString& _changeByEnter);
    void setShortcutsForStageplayEdit(const QString& _blockType, const QString& _shortcut,
                                      const QString& _jumpByTab, const QString& _jumpByEnter,
                                      const QString& _changeByTab, const QString& _changeByEnter);
    void setShortcutsForNovelEdit(const QString& _blockType, const QString& _shortcut,
                                  const QString& _jumpByTab, const QString& _jumpByEnter,
                                  const QString& _changeByTab, const QString& _changeByEnter);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
