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

    void showLanguagePage();

    void showThemePage();

    void showFinalPage();

signals:
    /**
     * @brief Пользователь выбрал язык приложения
     */
    void languageChanged(QLocale::Language _language);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
