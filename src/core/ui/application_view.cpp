#include "application_view.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>

#include <QVBoxLayout>


#include <QTimer>
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
    toolBar->setBackgroundColor(DesignSystem::color().primary());
    toolBar->setFixedHeight(static_cast<int>(DesignSystem::appBar().heightRegular()));

    navigator->setBackgroundColor(DesignSystem::color().primary());

    view->setBackgroundColor(DesignSystem::color().surface());

    splitter->setHandleColor(DesignSystem::color().primary());
}


// ****


ApplicationView::ApplicationView(QWidget* _parent)
    : QWidget(_parent),
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
    d->splitter->setSizes({1, 1});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);
}

void ApplicationView::showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view)
{
    d->toolBar->setCurrentWidget(_toolbar);
    d->navigator->setCurrentWidget(_navigator);
    d->view->setCurrentWidget(_view);
}

ApplicationView::~ApplicationView() = default;

} // namespace Ui
