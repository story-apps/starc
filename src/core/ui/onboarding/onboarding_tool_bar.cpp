#include "onboarding_tool_bar.h"

#include <ui/design_system/design_system.h>


namespace Ui
{

OnboardingToolBar::OnboardingToolBar(QWidget* _parent)
    : Widget(_parent)
{
    setBackgroundColor(DesignSystem::color().primary());
}

} // namespace Ui
