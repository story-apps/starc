#include "account_view.h"

#include <ui/design_system/design_system.h>


namespace Ui
{

AccountView::AccountView(QWidget* _parent)
    : Widget(_parent)
{

}

void AccountView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);
}

void AccountView::updateTranslations()
{

}

void AccountView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setBackgroundColor(DesignSystem::color().surface());
}

} // namespace Ui
