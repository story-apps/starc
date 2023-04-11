#include "add_comment_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QKeyEvent>
#include <QScrollArea>
#include <QVBoxLayout>


namespace Ui {

class AddCommentView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QScrollArea* content = nullptr;
    Widget* commentContainer = nullptr;
    TextField* comment = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;
};

AddCommentView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , commentContainer(new Widget(_parent))
    , comment(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , saveButton(new Button(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    UiHelper::initSpellingFor(comment);
    comment->setEnterMakesNewLine(true);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(saveButton);
}


// ****


AddCommentView::AddCommentView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->comment);

    d->comment->installEventFilter(this);

    auto commentContainerLayout = new QVBoxLayout(d->commentContainer);
    commentContainerLayout->setContentsMargins({});
    commentContainerLayout->setSpacing(0);
    commentContainerLayout->addWidget(d->comment);
    commentContainerLayout->addLayout(d->buttonsLayout);

    QWidget* contentWidget = new QWidget;
    d->content->setWidgetResizable(true);
    d->content->setWidget(contentWidget);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins({});
    contentLayout->setSpacing(0);
    contentLayout->addWidget(d->commentContainer);
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
    connect(d->saveButton, &Button::clicked, this, &AddCommentView::savePressed);
    connect(d->cancelButton, &Button::clicked, this, &AddCommentView::cancelPressed);
}

AddCommentView::~AddCommentView() = default;

void AddCommentView::setTopMargin(int _margin)
{
    d->content->widget()->layout()->setContentsMargins(
        0,
        std::min(static_cast<int>(std::max(0.0, _margin - DesignSystem::layout().px24())),
                 height() - d->commentContainer->height()),
        0, 0);
}

QString AddCommentView::comment() const
{
    return d->comment->text();
}

void AddCommentView::setComment(const QString& _comment)
{
    d->comment->setText(_comment);
}

bool AddCommentView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->comment && _event->type() == QEvent::KeyPress) {
        const auto keyEvent = static_cast<QKeyEvent*>(_event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit cancelPressed();
        } else if (keyEvent->modifiers().testFlag(Qt::ControlModifier)
                   && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
            emit savePressed();
            return true;
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void AddCommentView::updateTranslations()
{
    d->comment->setLabel(tr("Сomment"));
    d->cancelButton->setText(tr("Cancel"));
    d->saveButton->setText(tr("Save"));
}

void AddCommentView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    d->content->widget()->layout()->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);

    d->commentContainer->setBackgroundColor(
        ColorHelper::transparent(ColorHelper::nearby(Ui::DesignSystem::color().primary()), 0.3));
    d->commentContainer->layout()->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0,
                                                      Ui::DesignSystem::layout().px12());
    d->commentContainer->layout()->setSpacing(Ui::DesignSystem::layout().px12());

    d->comment->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->comment->setTextColor(Ui::DesignSystem::color().onPrimary());


    d->buttonsLayout->setContentsMargins(
        0, 0, Ui::DesignSystem::layout().px12() + Ui::DesignSystem::layout().px2(), 0);
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().accent());
    d->saveButton->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->saveButton->setTextColor(Ui::DesignSystem::color().accent());
}

} // namespace Ui
