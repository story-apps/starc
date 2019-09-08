#include "application_view.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>

#include <QVBoxLayout>


namespace Ui
{

class ApplicationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    StackWidget* toolBar = nullptr;
    StackWidget* navigator = nullptr;
    StackWidget* view = nullptr;

    Splitter* splitter = nullptr;
};

ApplicationView::Implementation::Implementation(QWidget* _parent)
    : toolBar(new StackWidget(_parent)),
      navigator(new StackWidget(_parent)),
      view(new StackWidget(_parent)),
      splitter(new Splitter(_parent))
{
}


// ****


ApplicationView::ApplicationView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    Widget* navigation = new Widget;
    QVBoxLayout* navigationLayout = new QVBoxLayout(navigation);
    navigationLayout->setContentsMargins({});
    navigationLayout->setSpacing(0);
    navigationLayout->addWidget(d->toolBar);
    navigationLayout->addWidget(d->navigator);

    d->splitter->addWidget(navigation);
    d->splitter->addWidget(d->view);
    d->splitter->setSizes({1, 4});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);

    designSystemChangeEvent(nullptr);
}

ApplicationView::~ApplicationView() = default;

void ApplicationView::showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view)
{
    d->toolBar->setCurrentWidget(_toolbar);
    d->navigator->setCurrentWidget(_navigator);
    d->view->setCurrentWidget(_view);
}

void ApplicationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->toolBar->setBackgroundColor(DesignSystem::color().primary());
    d->toolBar->setFixedHeight(static_cast<int>(DesignSystem::appBar().heightRegular()));

    d->navigator->setBackgroundColor(DesignSystem::color().primary());

    d->view->setBackgroundColor(DesignSystem::color().surface());

    d->splitter->setHandleColor(DesignSystem::color().primary());
}

} // namespace Ui
