#include "add_comment_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
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
    TextField* comment = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;
};

AddCommentView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
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
    connect(d->saveButton, &Button::clicked, this, &AddCommentView::savePressed);
    connect(d->cancelButton, &Button::clicked, this, &AddCommentView::cancelPressed);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

AddCommentView::~AddCommentView() = default;

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
        } else if (!keyEvent->modifiers().testFlag(Qt::ShiftModifier)
                   && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
            emit savePressed();
            return true;
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void AddCommentView::updateTranslations()
{
    d->comment->setLabel(tr("Add new comment"));
    d->cancelButton->setText(tr("Cancel"));
    d->saveButton->setText(tr("Save"));
}

void AddCommentView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    d->content->widget()->layout()->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);
    d->content->widget()->layout()->setSpacing(Ui::DesignSystem::layout().px12());

    d->comment->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->comment->setTextColor(Ui::DesignSystem::color().onPrimary());


    d->buttonsLayout->setContentsMargins(
        0, 0, Ui::DesignSystem::layout().px12() + Ui::DesignSystem::layout().px2(), 0);
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->saveButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->saveButton->setTextColor(Ui::DesignSystem::color().secondary());
}

} // namespace Ui
