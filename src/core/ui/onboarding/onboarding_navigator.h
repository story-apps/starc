#pragma once

#include <ui/widgets/stepper/stepper.h>


namespace Ui
{

/**
 * @brief Навигатор по посадочным страницам
 */
class OnboardingNavigator : public Stepper
{
    Q_OBJECT

public:
    explicit OnboardingNavigator(QWidget* _parent = nullptr);

protected:
    /**
     * @brief Обновить перевод
     */
    void updateTranslations() override;
};

} // namespace Ui
