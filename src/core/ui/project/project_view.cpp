#include "project_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <utils/helpers/color_helper.h>

#include <QVBoxLayout>


namespace Ui {

class ProjectView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Widget* defaultPage = nullptr;
    H6Label* defaultPageTitleLabel = nullptr;
    Body1Label* defaultPageBodyLabel = nullptr;
    Body1LinkLabel* defaultPageAddItemButton = nullptr;

    Widget* notImplementedPage = nullptr;
    H6Label* notImplementedPageTitleLabel = nullptr;
    Body1Label* notImplementedPageBodyLabel = nullptr;

    Widget* overlay = nullptr;
};

ProjectView::Implementation::Implementation(QWidget* _parent)
    : defaultPage(new Widget(_parent))
    , defaultPageTitleLabel(new H6Label(defaultPage))
    , defaultPageBodyLabel(new Body1Label(defaultPage))
    , defaultPageAddItemButton(new Body1LinkLabel(defaultPage))
    , notImplementedPage(new Widget(_parent))
    , notImplementedPageTitleLabel(new H6Label(notImplementedPage))
    , notImplementedPageBodyLabel(new Body1Label(notImplementedPage))
    , overlay(new Widget(_parent))
{
    defaultPageBodyLabel->setAlignment(Qt::AlignCenter);
    notImplementedPageBodyLabel->setAlignment(Qt::AlignCenter);
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->hide();

    {
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

    {
        QVBoxLayout* layout = new QVBoxLayout(notImplementedPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addStretch();
        layout->addWidget(notImplementedPageTitleLabel, 0, Qt::AlignHCenter);
        QHBoxLayout* bodyLayout = new QHBoxLayout;
        bodyLayout->setContentsMargins({});
        bodyLayout->setSpacing(0);
        bodyLayout->addStretch();
        bodyLayout->addWidget(notImplementedPageBodyLabel, 0, Qt::AlignHCenter);
        bodyLayout->addStretch();
        layout->addLayout(bodyLayout);
        layout->addStretch();
    }
}


// ****


ProjectView::ProjectView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(AnimationType::FadeThrough);

    addWidget(d->defaultPage);
    addWidget(d->notImplementedPage);

    showDefaultPage();

    connect(d->defaultPageAddItemButton, &Body1LinkLabel::clicked, this,
            &ProjectView::createNewItemPressed);
}

ProjectView::~ProjectView() = default;

void ProjectView::showDefaultPage()
{
    setCurrentWidget(d->defaultPage);
}

void ProjectView::showNotImplementedPage()
{
    setCurrentWidget(d->notImplementedPage);
}

void ProjectView::setActive(bool _active)
{
    const bool isVisible = !_active;
    d->overlay->setVisible(isVisible);
    if (isVisible) {
        d->overlay->raise();
    }
}

void ProjectView::resizeEvent(QResizeEvent* _event)
{
    StackWidget::resizeEvent(_event);

    d->overlay->resize(size());
}

void ProjectView::updateTranslations()
{
    d->defaultPageTitleLabel->setText(
        tr("Here will be an editor of the document you choose in the navigator (at left)."));
    d->defaultPageBodyLabel->setText(tr("Choose an item to edit, or"));
    d->defaultPageAddItemButton->setText(tr("create a new one"));

    d->notImplementedPageTitleLabel->setText(
        tr("Ooops... looks like editor of this document not implemented yet."));
    d->notImplementedPageBodyLabel->setText(
        tr("But don't worry, it will be here in one of the future updates!"));
}

void ProjectView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->defaultPage->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->defaultPageBodyLabel->setContentsMargins(
        0, static_cast<int>(Ui::DesignSystem::layout().px16()),
        static_cast<int>(Ui::DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->defaultPageTitleLabel,
             d->defaultPageBodyLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().surface());
        label->setTextColor(Ui::DesignSystem::color().onSurface());
    }
    d->defaultPageAddItemButton->setContentsMargins(
        0, static_cast<int>(Ui::DesignSystem::layout().px16()), 0, 0);
    d->defaultPageAddItemButton->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->defaultPageAddItemButton->setTextColor(Ui::DesignSystem::color().secondary());

    d->notImplementedPage->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->notImplementedPageBodyLabel->setContentsMargins(
        0, static_cast<int>(Ui::DesignSystem::layout().px16()),
        static_cast<int>(Ui::DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->notImplementedPageTitleLabel,
             d->notImplementedPageBodyLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().surface());
        label->setTextColor(Ui::DesignSystem::color().onSurface());
    }

    d->overlay->setBackgroundColor(
        ColorHelper::transparent(backgroundColor(), Ui::DesignSystem::inactiveItemOpacity()));
}

} // namespace Ui
