#include "onboarding_navigator.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>

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

    addStep({});
    addStep({});

    designSystemChangeEvent(nullptr);
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

void OnboardingNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
    setInactiveStepNumberBackgroundColor(
                ColorHelper::colorBetween(DesignSystem::color().primary(), DesignSystem::color().onPrimary()));
//    setInactiveStepNumberBackgroundColor(DesignSystem::color().shadow().lighter(300));
}

} // namespace Ui
