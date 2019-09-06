#include "onboarding_navigator.h"

#include <ui/design_system/design_system.h>

namespace {
    const int kLanguageStepIndex = 0;
    const int kThemeStepIndex = 1;
}


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

void OnboardingNavigator::showThemeStep()
{
    setCurrentStep(kThemeStepIndex);
}

void OnboardingNavigator::updateTranslations()
{
    setStepName(kLanguageStepIndex, tr("Choose language"));
    setStepName(kThemeStepIndex, tr("Setup user interface"));
}

} // namespace Ui
