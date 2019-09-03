#include "onboarding_view.h"

#include <ui/design_system/design_system.h>


namespace Ui
{

class OnboardingView::Implementation
{
public:
};


// ****


OnboardingView::OnboardingView(QWidget* _parent)
    : StackWidget(_parent)
{
    setBackgroundColor(DesignSystem::color().surface());
}

OnboardingView::~OnboardingView() = default;

} // namespace Ui
