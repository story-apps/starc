#include "screenplay_text_comments_view.h"

#include "screenplay_text_add_comment_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QTimer>
#include <QVBoxLayout>


namespace Ui
{

class ScreenplayTextCommentsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Tree* commentsView = nullptr;

    ScreenplayTextAddCommentWidget* addCommentWidget = nullptr;
    QColor addCommentColor;
};

ScreenplayTextCommentsView::Implementation::Implementation(QWidget* _parent)
    : commentsView(new Tree(_parent)),
      addCommentWidget(new ScreenplayTextAddCommentWidget(_parent))
{
}


// ****


ScreenplayTextCommentsView::ScreenplayTextCommentsView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->commentsView);
    addWidget(d->addCommentWidget);


    connect(d->addCommentWidget, &ScreenplayTextAddCommentWidget::savePressed, this, [this] {
        emit addCommentRequested(d->addCommentColor, d->addCommentWidget->comment());
        setCurrentWidget(d->commentsView);
    });
    connect(d->addCommentWidget, &ScreenplayTextAddCommentWidget::cancelPressed, this, [this] {
        setCurrentWidget(d->commentsView);
    });


    designSystemChangeEvent(nullptr);
}

ScreenplayTextCommentsView::~ScreenplayTextCommentsView() = default;

void ScreenplayTextCommentsView::showAddCommentView(const QColor& _withColor)
{
    d->addCommentColor = _withColor;
    d->addCommentWidget->setComment({});
    setCurrentWidget(d->addCommentWidget);
    QTimer::singleShot(animationDuration(), d->addCommentWidget, qOverload<>(&ScreenplayTextAddCommentWidget::setFocus));
}

void ScreenplayTextCommentsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->commentsView->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
