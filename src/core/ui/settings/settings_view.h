#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Представление настроек
 */
class SettingsView : public Widget
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget* _parent = nullptr);
    ~SettingsView() override;

    /**
     * @brief По возможности сфокусировать на экране заданный виджет
     */
    void showApplication();
    void showApplicationUserInterface();
    void showApplicationSaveAndBackups();
    void showComponents();
    void showComponentsScreenplay();
    void showShortcuts();

    //
    // Задание параметров приложения
    //
    void setApplicationLanguage(int _language);
    void setApplicationUseTypewriterSound(bool _use);
    void setApplicationUseSpellChecker(bool _use);
    void setApplicationSpellCheckerLanguage(const QString& _languageCode);
    void setApplicationTheme(int _theme);
    void setApplicationScaleFactor(qreal _scaleFactor);
    void setApplicationUseAutoSave(bool _use);
    void setApplicationSaveBackups(bool _save);
    void setApplicationBackupsFolder(const QString& _path);

    //
    // Задание параметров редактора сценария
    //
    void setScreenplayEditorDefaultTemplate(const QString& _templateId);
    void setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight);
    void setScreenplayEditorShowDialogueNumber(bool _show);
    void setScreenplayEditorHighlightCurrentLine(bool _highlight);
    //
    // Задание параметров навигатора сценария
    //
    void setScreenplayNavigatorShowSceneNumber(bool _show);
    void setScreenplayNavigatorShowSceneText(bool _show, int _lines);
    //
    // Задание параметров хронометража
    //
    void setScreenplayDurationType(int _type);
    void setScreenplayDurationByPageDuration(int _duration);
    void setScreenplayDurationByCharactersCharacters(int _characters);
    void setScreenplayDurationByCharactersIncludeSpaces(bool _include);
    void setScreenplayDurationByCharactersDuration(int _duration);

signals:
    /**
     * @brief Пользователь нажал кнопку выборя языка
     */
    void applicationLanguagePressed();

    /**
     * @brief Пользователь нажал кнопку выбора темы
     */
    void applicationThemePressed();

    //
    // Уведомление об изменении параметров приложения
    //
    void applicationUseTypewriterSoundChanged(bool _use);
    void applicationUseSpellCheckerChanged(bool _use);
    void applicationSpellCheckerLanguageChanged(const QString& _languageCode);
    void applicationScaleFactorChanged(qreal _scaleFactor);
    void applicationUseAutoSaveChanged(bool _use);
    void applicationSaveBackupsChanged(bool _save);
    void applicationBackupsFolderChanged(const QString& _path);

    //
    // Уведомление об изменении параметров редактора сценария
    //
    void screenplayEditorDefaultTemplateChanged(const QString& _templateId);
    void screenplayEditorShowSceneNumberChanged(bool _show, bool _atLeft, bool _atRight);
    void screenplayEditorShowDialogueNumberChanged(bool _show);
    void screenplayEditorHighlightCurrentLineChanged(bool _highlight);
    //
    // Уведомление об изменении параметров навигатора сценария
    //
    void screenplayNavigatorShowSceneNumberChanged(bool _show);
    void screenplayNavigatorShowSceneTextChanged(bool _show, int _lines);
    //
    // Уведомление об изменении параметров хронометража сценария
    //
    void screenplayDurationTypeChanged(int _type);
    void screenplayDurationByPageDurationChanged(int _duration);
    void screenplayDurationByCharactersCharactersChanged(int _characters);
    void screenplayDurationByCharactersIncludeSpacesChanged(bool _include);
    void screenplayDurationByCharactersDurationChanged(int _duration);

protected:
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
