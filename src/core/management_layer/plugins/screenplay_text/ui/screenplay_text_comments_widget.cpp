#include "screenplay_text_comments_widget.h"

namespace Ui
{

class ScreenplayTextCommentsWidget::Implementation
{
public:
};


// ****


ScreenplayTextCommentsWidget::ScreenplayTextCommentsWidget(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{

}

ScreenplayTextCommentsWidget::~ScreenplayTextCommentsWidget()
{

}

} // namespace Ui
