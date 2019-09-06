#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <QLocale>


namespace Ui
{

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

    /**
     * @brief Показать финальную страницу настроек
     */
    void showFinalPage();

signals:
    /**
     * @brief Пользователь выбрал язык приложения
     */
    void languageChanged(QLocale::Language _language);

    /**
     * @brief Пользователь хочет перейти к настройке темы
     */
    void showThemePageRequested();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем лейаут, когда обновилась дизайн система
     */
    void designSysemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
