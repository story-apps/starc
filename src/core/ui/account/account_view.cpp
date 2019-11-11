#include "account_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>

#include <QGridLayout>


namespace Ui
{

class AccountView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Card* userInfo = nullptr;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : userInfo(new Card(_parent))
{
}


// ****


AccountView::AccountView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QGridLayout* layout = new QGridLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->userInfo, 0, 0);
    layout->setRowStretch(1, 1);
    setLayout(layout);
}

AccountView::~AccountView() = default;

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
    d->userInfo->setBackgroundColor(DesignSystem::color().surface());
    layout()->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                                           Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toMargins());
}

} // namespace Ui
