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

    addStep("Choose preffered language");
    addStep("Setup application theme");
}

} // namespace Ui
