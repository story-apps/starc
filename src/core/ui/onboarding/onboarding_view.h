#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <QLocale>


namespace Ui {
enum class ApplicationTheme;

/**
 * @brief Контентные страницы посадочного экрана
 */
class OnboardingView : public StackWidget
{
    Q_OBJECT

public:
    explicit OnboardingView(QWidget* _parent = nullptr);
    ~OnboardingView() override;

    /**
     * @brief Показать страницу настройки языка
     */
    void showLanguagePage();

    /**
     * @brief Показать страницу настройки темы
     */
    void showThemePage();

signals:
    /**
     * @brief Пользователь выбрал язык приложения
     */
    void languageChanged(QLocale::Language _language);

    /**
     * @brief Пользователь хочет перейти к настройке темы
     */
    void showThemePageRequested();

    /**
     * @brief Пользователь выбрал тему приложения
     */
    void themeChanged(Ui::ApplicationTheme _theme);

    /**
     * @brief Пользователь изменил масштаб интерфейса
     */
    void scaleFactorChanged(qreal _scaleFactor);

    /**
     * @brief Пользователь хочет пропустить настройку
     */
    void skipOnboardingRequested();

    /**
     * @brief Пользователь хочет завершить настройку
     */
    void finishOnboardingRequested();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем внешний вид, когда обновилась дизайн система
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
