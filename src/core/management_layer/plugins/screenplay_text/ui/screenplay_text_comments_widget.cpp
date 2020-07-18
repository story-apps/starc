#include "screenplay_text_comments_widget.h"

#include "screenplay_text_add_comment_widget.h"

#include <QVBoxLayout>


namespace Ui
{

class ScreenplayTextCommentsWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    ScreenplayTextAddCommentWidget* addCommentWidget = nullptr;
};

ScreenplayTextCommentsWidget::Implementation::Implementation(QWidget* _parent)
    : addCommentWidget(new ScreenplayTextAddCommentWidget(_parent))
{
}


// ****


ScreenplayTextCommentsWidget::ScreenplayTextCommentsWidget(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setCurrentWidget(d->addCommentWidget);
}

ScreenplayTextCommentsWidget::~ScreenplayTextCommentsWidget()
{

}

} // namespace Ui
