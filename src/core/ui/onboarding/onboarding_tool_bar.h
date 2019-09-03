#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Панель инструментов посадочных страниц
 */
class OnboardingToolBar : public Widget
{
    Q_OBJECT

public:
    explicit OnboardingToolBar(QWidget *_parent = nullptr);
};

} // namespace Ui
