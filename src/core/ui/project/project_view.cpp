#include "project_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>

#include <QVBoxLayout>


namespace Ui
{

class ProjectView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Widget* defaultPage = nullptr;
    H6Label* defaultPageTitleLabel = nullptr;
    Body1Label* defaultPageBodyLabel = nullptr;
    Body1LinkLabel* defaultPageAddItemButton = nullptr;
};

ProjectView::Implementation::Implementation(QWidget* _parent)
    : defaultPage(new Widget(_parent)),
      defaultPageTitleLabel(new H6Label(defaultPage)),
      defaultPageBodyLabel(new Body1Label(defaultPage)),
      defaultPageAddItemButton(new Body1LinkLabel(defaultPage))
{
    defaultPageBodyLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout* layout = new QVBoxLayout(defaultPage);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(defaultPageTitleLabel, 0, Qt::AlignHCenter);
    QHBoxLayout* bodyLayout = new QHBoxLayout;
    bodyLayout->setContentsMargins({});
    bodyLayout->setSpacing(0);
    bodyLayout->addStretch();
    bodyLayout->addWidget(defaultPageBodyLabel, 0, Qt::AlignHCenter);
    bodyLayout->addWidget(defaultPageAddItemButton, 0, Qt::AlignHCenter);
    bodyLayout->addStretch();
    layout->addLayout(bodyLayout);
    layout->addStretch();
}


// ****


ProjectView::ProjectView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    showDefaultPage();

    connect(d->defaultPageAddItemButton, &Body1LinkLabel::clicked, this, &ProjectView::createNewItemPressed);

    designSystemChangeEvent(nullptr);
}

ProjectView::~ProjectView() = default;

void ProjectView::showDefaultPage()
{
    setCurrentWidget(d->defaultPage);
}

void ProjectView::updateTranslations()
{
    d->defaultPageTitleLabel->setText(tr("Here will be an editor of the document you choose in the navigator (at left)."));
    d->defaultPageBodyLabel->setText(tr("Choose an item to edit, or"));
    d->defaultPageAddItemButton->setText(tr("create a new one"));
}

void ProjectView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    d->defaultPage->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->defaultPageBodyLabel->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px16()),
                                                static_cast<int>(Ui::DesignSystem::layout().px4()), 0);
    for (auto label : QVector<Widget*>{ d->defaultPageTitleLabel, d->defaultPageBodyLabel }) {
        label->setBackgroundColor(Ui::DesignSystem::color().surface());
        label->setTextColor(Ui::DesignSystem::color().onSurface());
    }
    d->defaultPageAddItemButton->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px16()),
                                                    0, 0);
    d->defaultPageAddItemButton->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->defaultPageAddItemButton->setTextColor(Ui::DesignSystem::color().secondary());
}

} // namespace Ui
