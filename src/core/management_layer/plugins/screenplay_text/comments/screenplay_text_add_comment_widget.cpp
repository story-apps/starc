#include "screenplay_text_add_comment_widget.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/button/button.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>

#include <QScrollArea>
#include <QVBoxLayout>


namespace Ui
{

class ScreenplayTextAddCommentWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QScrollArea* content = nullptr;
    TextField* comment = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;
};

ScreenplayTextAddCommentWidget::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      comment(new TextField(_parent)),
      cancelButton(new Button(_parent)),
      saveButton(new Button(_parent))
{

}


// ****


ScreenplayTextAddCommentWidget::ScreenplayTextAddCommentWidget(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    d->content->setPalette(palette);
    d->content->setFrameShape(QFrame::NoFrame);
    d->content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->content->setVerticalScrollBar(new ScrollBar);

    d->comment->setEnterMakesNewLine(true);

    d->buttonsLayout = new QHBoxLayout;
    d->buttonsLayout->setContentsMargins({});
    d->buttonsLayout->setSpacing(0);
    d->buttonsLayout->addStretch();
    d->buttonsLayout->addWidget(d->cancelButton);
    d->buttonsLayout->addWidget(d->saveButton);

    QWidget* contentWidget = new QWidget;
    d->content->setWidgetResizable(true);
    d->content->setWidget(contentWidget);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins({});
    contentLayout->setSpacing(0);
    contentLayout->addWidget(d->comment);
    contentLayout->addLayout(d->buttonsLayout);
    contentLayout->addStretch();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);


    connect(d->comment, &TextField::cursorPositionChanged, this, [this] {
        if (!d->comment->hasFocus()) {
            return;
        }

        d->content->ensureVisible(0, d->comment->pos().y() + d->comment->cursorRect().bottom());
    });


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextAddCommentWidget::~ScreenplayTextAddCommentWidget() = default;

void ScreenplayTextAddCommentWidget::updateTranslations()
{
    d->comment->setLabel(tr("Add new comment"));
    d->cancelButton->setText(tr("Cancel"));
    d->saveButton->setText(tr("Save"));
}

void ScreenplayTextAddCommentWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    d->content->widget()->layout()->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);
    d->content->widget()->layout()->setSpacing(Ui::DesignSystem::layout().px12());

    d->comment->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->comment->setTextColor(Ui::DesignSystem::color().onPrimary());


    d->buttonsLayout->setContentsMargins(0, 0, Ui::DesignSystem::layout().px12() + Ui::DesignSystem::layout().px2(), 0);
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->saveButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->saveButton->setTextColor(Ui::DesignSystem::color().secondary());
}

} // nmaespace Ui
