#pragma once

#include <ui/design_system/design_system.h>

#include <QLocale>
#include <QObject>

namespace Ui {
    enum class ApplicationTheme;
}


namespace ManagementLayer
{

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject* _parent, QWidget* _parentWidget);
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
    void applicationThemeChanged(Ui::ApplicationTheme _theme);
    void applicationCustomThemeColorsChanged(const Ui::DesignSystem::Color& _color);
    void applicationScaleFactorChanged(qreal _scaleFactor);
    void applicationUseAutoSaveChanged(bool _use);
    void applicationSaveBackupsChanged(bool _save);
    void applicationBackupsFolderChanged(const QString& _path);

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
    void setApplicationUseSpellChecker(bool _use);
    void setApplicationSpellCheckerLanguage(const QString& _languageCode);
    void setApplicationTheme(Ui::ApplicationTheme _theme);
    void setApplicationCustomThemeColors(const Ui::DesignSystem::Color& _color);
    void setApplicationScaleFactor(qreal _scaleFactor);
    void setApplicationUseAutoSave(bool _use);
    void setApplicationSaveBackups(bool _save);
    void setApplicationBackupsFolder(const QString& _path);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
