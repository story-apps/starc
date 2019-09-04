#include "onboarding_navigator.h"

#include <ui/design_system/design_system.h>


namespace Ui
{

OnboardingNavigator::OnboardingNavigator(QWidget* _parent)
    : Stepper(_parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
    setInactiveStepNumberBackgroundColor(DesignSystem::color().primary().lighter(300));

    addStep({});
    addStep({});
    updateTranslations();
}

void OnboardingNavigator::updateTranslations()
{
    setStepName(0, tr("Choose language"));
    setStepName(1, tr("Setup user interface"));
}

} // namespace Ui
