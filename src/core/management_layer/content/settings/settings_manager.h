#pragma once

#include <ui/design_system/design_system.h>

#include <QLocale>
#include <QObject>

namespace Ui {
    enum class ApplicationTheme;
}


namespace ManagementLayer
{

/**
 * @brief Менеджер экрана настроек
 */
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    SettingsManager(QObject* _parent, QWidget* _parentWidget);
    ~SettingsManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

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
    void applicationUseSpellCheckerChanged(bool _use);
    void applicationSpellCheckerLanguageChanged(const QString& _languageCode);
    void applicationThemeChanged(Ui::ApplicationTheme _theme);
    void applicationCustomThemeColorsChanged(const Ui::DesignSystem::Color& _color);
    void applicationScaleFactorChanged(qreal _scaleFactor);
    void applicationUseAutoSaveChanged(bool _use);
    void applicationSaveBackupsChanged(bool _save);
    void applicationBackupsFolderChanged(const QString& _path);

    /**
     * @brief Изменились параметры компонентов
     */
    void screenplayEditorChanged(const QStringList& _changedSettingsKeys);
    void screenplayNavigatorChanged();
    void screenplayDurationChanged();

protected:
    /**
     * @brief Реализуем фильтр на событие смены языка и дизайн системы, чтобы обновить значения в представлении
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    //
    // Сохранение параметров приложения
    //
    void setApplicationLanguage(int _language);
    void setApplicationUseTypeWriterSound(bool _use);
    void setApplicationUseSpellChecker(bool _use);
    void setApplicationSpellCheckerLanguage(const QString& _languageCode);
    void loadSpellingDictionary(const QString& _languageCode);
    void loadSpellingDictionaryAffFile(const QString& _languageCode);
    void loadSpellingDictionaryDicFile(const QString& _languageCode);
    void setApplicationTheme(Ui::ApplicationTheme _theme);
    void setApplicationCustomThemeColors(const Ui::DesignSystem::Color& _color);
    void setApplicationScaleFactor(qreal _scaleFactor);
    void setApplicationUseAutoSave(bool _use);
    void setApplicationSaveBackups(bool _save);
    void setApplicationBackupsFolder(const QString& _path);

    //
    // Сохранение параметров редактора сценария
    //
    void setScreenplayEditorDefaultTemplate(const QString& _templateId);
    void setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight);
    void setScreenplayEditorShowDialogueNumber(bool _show);
    void setScreenplayEditorHighlightCurrentLine(bool _highlight);
    //
    // Сохранение параметров навигатора сценария
    //
    void setScreenplayNavigatorShowSceneNumber(bool _show);
    void setScreenplayNavigatorShowSceneText(bool _show, int _lines);
    //
    // Сохранение параметров хронометража сценария
    //
    void setScreenplayDurationType(int _type);
    void setScreenplayDurationByPageDuration(int _duration);
    void setScreenplayDurationByCharactersCharacters(int _characters);
    void setScreenplayDurationByCharactersIncludeSpaces(bool _include);
    void setScreenplayDurationByCharactersDuration(int _duration);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
