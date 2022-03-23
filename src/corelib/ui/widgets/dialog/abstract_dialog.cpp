#include "abstract_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/resizable_widget/resizable_widget.h>

#include <QApplication>
#include <QGridLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>


class AbstractDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Настроить таймер мониторинга активности приложения, если ещё не настроен
     */
    void initApplicationActivityChangingTimer();

    /**
     * @brief Анимировать отображение
     */
    void animateShow(const QPoint& _pos);

    /**
     * @brief Анимировать скрытие
     */
    void animateHide(const QPoint& _pos);


    static QTimer sApplicationActivityChangingTimer;

    QVBoxLayout* layout = nullptr;
    ResizableWidget* content = nullptr;
    bool isContentMaximumWidthChanged = false;
    bool isContentMinimumWidthChanged = false;
    H6Label* title = nullptr;
    QGridLayout* contentsLayout = nullptr;
    Button* acceptButton = nullptr;
    Button* rejectButton = nullptr;

    QVariantAnimation opacityAnimation;
    QVariantAnimation contentPosAnimation;
    QPixmap contentPixmap;
};

AbstractDialog::Implementation::Implementation(QWidget* _parent)
    : content(new ResizableWidget(_parent))
    , title(new H6Label(_parent))
    , contentsLayout(new QGridLayout)
{
    title->hide();

    layout = new QVBoxLayout(content);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(title);
    layout->addLayout(contentsLayout);

    content->setResizingActive(true);

    opacityAnimation.setDuration(220);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    contentPosAnimation.setDuration(220);
    contentPosAnimation.setEasingCurve(QEasingCurve::OutQuad);

    initApplicationActivityChangingTimer();
}

void AbstractDialog::Implementation::initApplicationActivityChangingTimer()
{
    const int applicationActivityChangingTimerInterval = 1000;
    if (sApplicationActivityChangingTimer.interval() == applicationActivityChangingTimerInterval
        && sApplicationActivityChangingTimer.isSingleShot()) {
        return;
    }

    sApplicationActivityChangingTimer.setInterval(applicationActivityChangingTimerInterval);
    sApplicationActivityChangingTimer.setSingleShot(true);
    connect(qApp, &QApplication::applicationStateChanged, &sApplicationActivityChangingTimer,
            [](Qt::ApplicationState _state) {
                if (_state == Qt::ApplicationActive) {
                    sApplicationActivityChangingTimer.start();
                }
            });
}

void AbstractDialog::Implementation::animateShow(const QPoint& _pos)
{
    //
    // Прозрачность
    //
    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.start();

    //
    // Позиция контента
    //
    const qreal movementDelta = 40;
    contentPosAnimation.setStartValue(QPointF(_pos - QPoint(0, static_cast<int>(movementDelta))));
    contentPosAnimation.setEndValue(QPointF(_pos));
    contentPosAnimation.start();
}

void AbstractDialog::Implementation::animateHide(const QPoint& _pos)
{
    contentPosAnimation.setEndValue(QPointF(_pos));

    opacityAnimation.setStartValue(1.0);
    opacityAnimation.setEndValue(0.0);
    opacityAnimation.start();
}

QTimer AbstractDialog::Implementation::sApplicationActivityChangingTimer;


// ****


AbstractDialog::AbstractDialog(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    Q_ASSERT(_parent);

    _parent->installEventFilter(this);

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);
    layout->addWidget(d->content, 1, 1);
    layout->setRowStretch(2, 1);
    layout->setColumnStretch(2, 1);

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->contentPosAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->contentPosAnimation, &QVariantAnimation::finished, this, [this] {
        d->content->show();
        focusedWidgetAfterShow()->setFocus();
    });

    designSystemChangeEvent(nullptr);
}

AbstractDialog::~AbstractDialog() = default;

void AbstractDialog::showDialog()
{
    if (parentWidget() == nullptr) {
        return;
    }

    //
    // Установим обрабочик событий, чтобы перехватывать потерю фокуса и возвращать его в диалог
    //
    focusedWidgetAfterShow()->installEventFilter(this);
    lastFocusableWidget()->installEventFilter(this);

    //
    // Конфигурируем геометрию диалога
    //
    move(0, 0);
    resize(parentWidget()->size());

    //
    // Сохраняем изображение контента и прячем сам виджет
    //
    d->contentPixmap = d->content->grab();
    d->content->hide();

    //
    // Запускаем отображение
    //
    d->animateShow(d->content->pos());
    setFocus();
    show();
}

void AbstractDialog::hideDialog()
{
    focusedWidgetAfterShow()->removeEventFilter(this);
    lastFocusableWidget()->removeEventFilter(this);

    d->contentPixmap = d->content->grab();
    d->content->hide();
    d->animateHide(d->content->pos());

    QTimer::singleShot(d->opacityAnimation.duration(), this, &AbstractDialog::hide);
}

void AbstractDialog::setContentMinimumWidth(int _width)
{
    d->content->setMinimumWidth(_width);
    d->isContentMinimumWidthChanged = true;
    updateGeometry();
}

void AbstractDialog::setContentMaximumWidth(int _width)
{
    d->content->setMaximumWidth(_width);
    d->isContentMaximumWidthChanged = true;
    updateGeometry();
}

void AbstractDialog::setContentFixedWidth(int _width)
{
    setContentMinimumWidth(_width);
    setContentMaximumWidth(_width);
}

void AbstractDialog::setAcceptButton(Button* _button)
{
    d->acceptButton = _button;
}

void AbstractDialog::setRejectButton(Button* _button)
{
    d->rejectButton = _button;
}

QString AbstractDialog::title() const
{
    return d->title->text();
}

void AbstractDialog::setTitle(const QString& _title)
{
    d->title->setVisible(!_title.isEmpty());
    d->title->setText(_title);

    //
    // Нужно обновить отступ между заголовком и поясняющим текстом
    //
    designSystemChangeEvent(nullptr);
}

QGridLayout* AbstractDialog::contentsLayout() const
{
    return d->contentsLayout;
}

bool AbstractDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::Resize && _watched == parentWidget()) {
        auto resizeEvent = static_cast<QResizeEvent*>(_event);
        resize(resizeEvent->size());
    } else if (_event->type() == QEvent::FocusOut
               && (QApplication::focusWidget() == nullptr
                   || !findChildren<QWidget*>().contains(QApplication::focusWidget()))) {
        //
        // Устанавливаем фокус отложенно, чтобы не впасть в рекурсивный цикл
        //
        if (_watched == lastFocusableWidget()) {
            QMetaObject::invokeMethod(focusedWidgetAfterShow(), qOverload<>(&QWidget::setFocus),
                                      Qt::QueuedConnection);
        } else if (_watched == focusedWidgetAfterShow()) {
            QMetaObject::invokeMethod(lastFocusableWidget(), qOverload<>(&QWidget::setFocus),
                                      Qt::QueuedConnection);
        }
    }

    return QWidget::eventFilter(_watched, _event);
}

bool AbstractDialog::event(QEvent* _event)
{
    if (_event->type() == QEvent::KeyPress) {
        const auto keyEvent = static_cast<QKeyEvent*>(_event);
        if (d->acceptButton != nullptr && d->acceptButton->isEnabled()
            && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
            d->acceptButton->click();
            return true;
        } else if (d->rejectButton != nullptr && d->rejectButton->isEnabled()
                   && keyEvent->key() == Qt::Key_Escape) {
            d->rejectButton->click();
            return true;
        }
    }

    return Widget::event(_event);
}

void AbstractDialog::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setOpacity(d->opacityAnimation.currentValue().toReal());

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), Ui::DesignSystem::color().shadow());

    //
    // Если надо рисуем образ контента
    //
    if (!d->content->isVisible()) {
        painter.drawPixmap(d->contentPosAnimation.currentValue().toPointF(), d->contentPixmap);
    }
}

void AbstractDialog::mousePressEvent(QMouseEvent* _event)
{
    //
    // Если пользователь кликнул вне области контента диалога,
    // и при этом окно не было активировано посредством клика в этой области,
    // и есть кнопка отмены, то используем её
    //
    if (!d->title->rect().contains(d->title->mapFromGlobal(_event->globalPos()))
        && !d->content->rect().contains(d->content->mapFromGlobal(_event->globalPos()))
        && !d->sApplicationActivityChangingTimer.isActive() && d->rejectButton != nullptr
        && d->rejectButton->isEnabled()) {
        d->rejectButton->click();
    }
}

void AbstractDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    if (!d->isContentMinimumWidthChanged) {
        d->content->setMinimumWidth(static_cast<int>(Ui::DesignSystem::dialog().minimumWidth()));
    }
    if (!d->isContentMaximumWidthChanged) {
        d->content->setMaximumWidth(static_cast<int>(Ui::DesignSystem::dialog().maximumWidth()));
    }
    d->content->setBackgroundColor(Ui::DesignSystem::color().background());
    d->title->setBackgroundColor(Qt::transparent);
    d->title->setTextColor(Ui::DesignSystem::color().onBackground());
    d->title->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
}

void AbstractDialog::show()
{
    QWidget::show();
}

void AbstractDialog::hide()
{
    QWidget::hide();
}
