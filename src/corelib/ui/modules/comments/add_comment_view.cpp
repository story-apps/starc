#include "add_comment_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QKeyEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

class AddCommentView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить отступ сверху над полем ввода
     */
    void updateTopMargin();


    QScrollArea* content = nullptr;
    Widget* commentContainer = nullptr;
    TextField* comment = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;

    int commentTopMargin = 0;
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

    comment->setEnterMakesNewLine(true);
    UiHelper::initSpellingFor(comment);
    UiHelper::initOptionsFor(comment);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(saveButton);
}

void AddCommentView::Implementation::updateTopMargin()
{
    content->widget()->layout()->setContentsMargins(
        0,
        std::min(static_cast<int>(std::max(0.0, commentTopMargin - DesignSystem::layout().px24())),
                 content->height() - commentContainer->height()),
        0, 0);
}


// ****


AddCommentView::AddCommentView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->comment);

    d->content->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    if (d->commentTopMargin == _margin) {
        return;
    }

    d->commentTopMargin = _margin;
    d->updateTopMargin();
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
    if (_watched == d->comment) {
        //
        // На нажатие клавиш, испускаем сигналы сохранения/отмены
        //
        if (_event->type() == QEvent::KeyPress) {
            const auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Escape) {
                emit cancelPressed();
            } else if (keyEvent->modifiers().testFlag(Qt::ControlModifier)
                       && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
                emit savePressed();
                return true;
            }
        }
        //
        // При изменении размера поля ввода комментария, корректируем позицию поля, для удобства
        //
        else if (_event->type() == QEvent::Resize) {
            const auto resizeEvent = static_cast<QResizeEvent*>(_event);
            //
            // ... делаем коррекцию чуть отложено, чтобы все лейауты и полоса прокрутки
            //     успели обновиться
            //
            QTimer::singleShot(
                10, this,
                [this,
                 sizeChange = resizeEvent->size().height() - resizeEvent->oldSize().height()] {
                    //
                    // ... если появилась прорутка
                    //
                    bool needToScroll = false;
                    if (d->content->verticalScrollBar()->maximum() > 0) {
                        //
                        // ... пробуем скорректировать верхний отступ
                        //
                        if (d->commentTopMargin > sizeChange) {
                            d->commentTopMargin -= sizeChange;
                            d->updateTopMargin();
                        }
                        //
                        // ... либо двигаем прокрутку в самый низ, чтобы кнопки были у пользователя
                        //     на виду и не уезжали вниз
                        //
                        else {
                            if (d->commentTopMargin > 0) {
                                d->commentTopMargin = 0;
                                d->updateTopMargin();
                            }
                            needToScroll = true;
                        }
                    }
                    //
                    // ... вручную настраиваем отображение полосы прокрутки после того, как мы
                    //     скорректировали положение поля для ввода комментария
                    //
                    d->content->setVerticalScrollBarPolicy(d->commentContainer->height() > height()
                                                               ? Qt::ScrollBarAlwaysOn
                                                               : Qt::ScrollBarAlwaysOff);
                    if (needToScroll) {
                        d->content->verticalScrollBar()->setValue(
                            d->content->verticalScrollBar()->maximum());
                    }
                });
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
    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->saveButton, UiHelper::DialogAccept);
}

} // namespace Ui
