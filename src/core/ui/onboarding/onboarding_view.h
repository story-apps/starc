#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
