#include "screenplay_text_structure_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/tree/tree.h>

#include <QScrollArea>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui
{

class ScreenplayTextStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsSmallLabel* backIcon = nullptr;
    CaptionLabel* backText = nullptr;
    QHBoxLayout* topLayout = nullptr;
    Tree* content = nullptr;
};

ScreenplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsSmallLabel(_parent)),
      backText(new CaptionLabel(_parent)),
      topLayout(new QHBoxLayout),
      content(new Tree(_parent))
{
    backIcon->setText("\uf141");
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(backIcon);
    topLayout->addWidget(backText, 1);
}


// ****


ScreenplayTextStructureView::ScreenplayTextStructureView(QWidget* _parent)
    : AbstractNavigator(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(d->topLayout);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->backIcon, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextStructureView::~ScreenplayTextStructureView() = default;

void ScreenplayTextStructureView::updateTranslations()
{
    d->backText->setText("Back to navigator");
}

void ScreenplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    for (auto widget : QVector<Widget*>{d->backIcon, d->backText, d->content}) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(DesignSystem::color().onPrimary());
    }

    d->topLayout->setContentsMargins(Ui::DesignSystem::layout().px16(),
                                     Ui::DesignSystem::layout().px4(),
                                     Ui::DesignSystem::layout().px12(),
                                     0);
    d->topLayout->setSpacing(Ui::DesignSystem::layout().px4());
}

} // namespace Ui
