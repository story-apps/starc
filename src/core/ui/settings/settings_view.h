#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

class QAbstractItemModel;
class HierarchicalModel;


namespace Ui {

enum class ApplicationTheme;

/**
 * @brief Представление настроек
 */
class SettingsView : public StackWidget
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget* _parent = nullptr);
    ~SettingsView() override;

    /**
     * @brief Показать основную страницу
     */
    void showDefaultPage();

    /**
     * @brief По возможности сфокусировать на экране заданный виджет
     */
    void showApplication();
    void showApplicationUserInterface();
    void showApplicationSaveAndBackups();
    void showApplicationTextEditing();
    void showComponents();
    void showComponentsSimpleText();
    void showComponentsScreenplay();
    void showComponentsComicBook();
    void showComponentsAudioplay();
    void showComponentsStageplay();
    void showShortcuts();

    //
    // Задание параметров приложения
    //
    void setApplicationLanguage(int _language);
    void setApplicationScaleFactor(qreal _scaleFactor);
    void setApplicationUseAutoSave(bool _use);
    void setApplicationSaveBackups(bool _save);
    void setApplicationBackupsFolder(const QString& _path);
    void setApplicationShowDocumentsPages(bool _show);
    void setApplicationUseTypewriterSound(bool _use);
    void setApplicationUseSpellChecker(bool _use);
    void setApplicationSpellCheckerLanguage(const QString& _languageCode);
    void setApplicationHighlightCurrentLine(bool _highlight);
    void setApplicationFocusCurrentParagraph(bool _focus);
    void setApplicationUseTypewriterScrolling(bool _use);
    void setApplicationReplaceThreeDotsWithEllipsis(bool _replace);
    void setApplicationUseSmartQuotes(bool _use);

    //
    // Задание параметров редактора текста
    //
    void setSimpleTextAvailable(bool _available);
    //
    void setSimpleTextEditorDefaultTemplate(const QString& _templateId);
    //
    void setSimpleTextNavigatorShowSceneText(bool _show, int _lines);

    //
    // Задание параметров редактора сценария
    //
    void setScreenplayAvailable(bool _available);
    //
    void setScreenplayEditorDefaultTemplate(const QString& _templateId);
    void setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight);
    void setScreenplayEditorShowDialogueNumber(bool _show);
    void setScreenplayEditorContinueDialogue(bool _continue);
    void setScreenplayEditorCorrectTextOnPageBreaks(bool _correct);
    void setScreenplayEditorUseCharactersFromText(bool _use);
    void setScreenplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setScreenplayNavigatorShowBeats(bool _show, bool _withFullText);
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
    // Задание параметров редактора комикса
    //
    void setComicBookAvailable(bool _available);
    //
    void setComicBookEditorDefaultTemplate(const QString& _templateId);
    void setComicBookEditorUseCharactersFromText(bool _use);
    void setComicBookEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setComicBookNavigatorShowSceneText(bool _show, int _lines);

    //
    // Задание параметров редактора подкаста
    //
    void setAudioplayAvailable(bool _available);
    //
    void setAudioplayEditorDefaultTemplate(const QString& _templateId);
    void setAudioplayEditorShowBlockNumber(bool _show, bool _continue);
    void setAudioplayEditorUseCharactersFromText(bool _use);
    void setAudioplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setAudioplayNavigatorShowSceneNumber(bool _show);
    void setAudioplayNavigatorShowSceneText(bool _show, int _lines);
    //
    void setAudioplayDurationByWordsWords(int _words);
    void setAudioplayDurationByWordsDuration(int _duration);

    //
    // Задание параметров редактора пьесы
    //
    void setStageplayAvailable(bool _available);
    //
    void setStageplayEditorDefaultTemplate(const QString& _templateId);
    void setStageplayEditorUseCharactersFromText(bool _use);
    void setStageplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show);
    //
    void setStageplayNavigatorShowSceneNumber(bool _show);
    void setStageplayNavigatorShowSceneText(bool _show, int _lines);

    //
    // Задание параметров горячих клавиш
    //
    void setShortcutsForScreenplayModel(HierarchicalModel* _model);

signals:
    /**
     * @brief Пользователь нажал кнопку выборя языка
     */
    void applicationLanguagePressed();

    //
    // Уведомление об изменении параметров приложения
    //
    void applicationThemePressed(Ui::ApplicationTheme _theme);
    void applicationScaleFactorChanged(qreal _scaleFactor);
    void applicationUseAutoSaveChanged(bool _use);
    void applicationSaveBackupsChanged(bool _save);
    void applicationBackupsFolderChanged(const QString& _path);
    void applicationShowDocumentsPagesChanged(bool _show);
    void applicationUseTypewriterSoundChanged(bool _use);
    void applicationUseSpellCheckerChanged(bool _use);
    void applicationSpellCheckerLanguageChanged(const QString& _languageCode);
    void applicationHighlightCurentLineChanged(bool _highlight);
    void applicationFocusCurrentParagraphChanged(bool _focus);
    void applicationUseTypewriterScrollingChanged(bool _use);
    void applicationReplaceThreeDotsWithEllipsisChanged(bool _replace);
    void applicationUseSmartQuotesChanged(bool _use);

    //
    // Уведомление об изменении параметров редактора текста
    //
    void simpleTextAvailableChanged(bool _available);
    //
    void simpleTextEditorDefaultTemplateChanged(const QString& _templateId);
    //
    void simpleTextNavigatorShowSceneTextChanged(bool _show, int _lines);

    //
    // Уведомление об изменении параметров редактора сценария
    //
    void screenplayAvailableChanged(bool _available);
    //
    void screenplayEditorDefaultTemplateChanged(const QString& _templateId);
    void screenplayEditorShowSceneNumberChanged(bool _show, bool _atLeft, bool _atRight);
    void screenplayEditorShowDialogueNumberChanged(bool _show);
    void screenplayEditorContinueDialogueChanged(bool _continue);
    void screenplayEditorCorrectTextOnPageBreaksChanged(bool _correct);
    void screenplayEditorUseCharactersFromTextChanged(bool _use);
    void screenplayEditorShowCharacterSuggestionsInEmptyBlockChanged(bool _show);
    //
    void screenplayNavigatorShowBeatsChanged(bool _show, bool _withFullText);
    void screenplayNavigatorShowSceneNumberChanged(bool _show);
    void screenplayNavigatorShowSceneTextChanged(bool _show, int _lines);
    //
    void screenplayDurationTypeChanged(int _type);
    void screenplayDurationByPageDurationChanged(int _duration);
    void screenplayDurationByCharactersCharactersChanged(int _characters);
    void screenplayDurationByCharactersIncludeSpacesChanged(bool _include);
    void screenplayDurationByCharactersDurationChanged(int _duration);
    void screenplayDurationConfigurablePerParagraphForActionChanged(qreal _duration);
    void screenplayDurationConfigurablePerEvery50ForActionChanged(qreal _duration);
    void screenplayDurationConfigurablePerParagraphForDialogueChanged(qreal _duration);
    void screenplayDurationConfigurablePerEvery50ForDialogueChanged(qreal _duration);
    void screenplayDurationConfigurablePerParagraphForSceneHeadingChanged(qreal _duration);
    void screenplayDurationConfigurablePerEvery50ForSceneHeadingChanged(qreal _duration);

    //
    // Уведомление об изменении параметров редактора комикса
    //
    void comicBookAvailableChanged(bool _available);
    //
    void comicBookEditorDefaultTemplateChanged(const QString& _templateId);
    void comicBookEditorUseCharactersFromTextChanged(bool _use);
    void comicBookEditorShowCharacterSuggestionsInEmptyBlockChanged(bool _show);
    //
    void comicBookNavigatorShowSceneTextChanged(bool _show, int _lines);

    //
    // Уведомление об изменении параметров редактора аудиопостановки
    //
    void audioplayAvailableChanged(bool _available);
    //
    void audioplayEditorDefaultTemplateChanged(const QString& _templateId);
    void audioplayEditorShowBlockNumberChanged(bool _show, bool _continue);
    void audioplayEditorUseCharactersFromTextChanged(bool _use);
    void audioplayEditorShowCharacterSuggestionsInEmptyBlockChanged(bool _show);
    //
    void audioplayNavigatorShowSceneNumberChanged(bool _show);
    void audioplayNavigatorShowSceneTextChanged(bool _show, int _lines);
    //
    void audioplayDurationByWordsWordsChanged(int _words);
    void audioplayDurationByWordsDurationChanged(int _duration);

    //
    // Уведомление об изменении параметров редактора пьес
    //
    void stageplayAvailableChanged(bool _available);
    //
    void stageplayEditorDefaultTemplateChanged(const QString& _templateId);
    void stageplayEditorShowBlockNumberChanged(bool _show, bool _continue);
    void stageplayEditorUseCharactersFromTextChanged(bool _use);
    void stageplayEditorShowCharacterSuggestionsInEmptyBlockChanged(bool _show);
    //
    void stageplayNavigatorShowSceneNumberChanged(bool _show);
    void stageplayNavigatorShowSceneTextChanged(bool _show, int _lines);

    //
    // Редактирование шаблонов
    //
    // ... простой текстовый документ
    //
    void editCurrentSimpleTextEditorTemplateRequested(const QString& _templateId);
    void duplicateCurrentSimpleTextEditorTemplateRequested(const QString& _templateId);
    void saveToFileCurrentSimpleTextEditorTemplateRequested(const QString& _templateId);
    void removeCurrentSimpleTextEditorTemplateRequested(const QString& _templateId);
    void loadFromFileSimpleTextEditorTemplateRequested();
    //
    // ... сценарий
    //
    void editCurrentScreenplayEditorTemplateRequested(const QString& _templateId);
    void duplicateCurrentScreenplayEditorTemplateRequested(const QString& _templateId);
    void saveToFileCurrentScreenplayEditorTemplateRequested(const QString& _templateId);
    void removeCurrentScreenplayEditorTemplateRequested(const QString& _templateId);
    void loadFromFileScreenplayEditorTemplateRequested();
    //
    // ... комикс
    //
    void editCurrentComicBookEditorTemplateRequested(const QString& _templateId);
    void duplicateCurrentComicBookEditorTemplateRequested(const QString& _templateId);
    void saveToFileCurrentComicBookEditorTemplateRequested(const QString& _templateId);
    void removeCurrentComicBookEditorTemplateRequested(const QString& _templateId);
    void loadFromFileComicBookEditorTemplateRequested();
    //
    // ... аудиопостановка
    //
    void editCurrentAudioplayEditorTemplateRequested(const QString& _templateId);
    void duplicateCurrentAudioplayEditorTemplateRequested(const QString& _templateId);
    void saveToFileCurrentAudioplayEditorTemplateRequested(const QString& _templateId);
    void removeCurrentAudioplayEditorTemplateRequested(const QString& _templateId);
    void loadFromFileAudioplayEditorTemplateRequested();
    //
    // ... пьеса
    //
    void editCurrentStageplayEditorTemplateRequested(const QString& _templateId);
    void duplicateCurrentStageplayEditorTemplateRequested(const QString& _templateId);
    void saveToFileCurrentStageplayEditorTemplateRequested(const QString& _templateId);
    void removeCurrentStageplayEditorTemplateRequested(const QString& _templateId);
    void loadFromFileStageplayEditorTemplateRequested();

    //
    // Параметры горячих клавиш
    //
    void shortcutsForScreenplayEditorChanged(const QString& _blockType, const QString& _shortcut,
                                             const QString& _jumpByTab, const QString& _jumpByEnter,
                                             const QString& _changeByTab,
                                             const QString& _changeByEnter);

protected:
    /**
     * @brief Переопределяем для обновления размеров таблиц
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
