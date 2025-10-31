#include "connection_status_tool_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QVariantAnimation>

namespace Ui {

namespace {
constexpr int kUnsetPosition = -1;
}

class ConnectionStatusToolBar::Implementation
{
public:
    explicit Implementation(ConnectionStatusToolBar* _q);

    /**
     * @brief Скорректировать положение панели относительно родительского виджета
     */
    void correctPosition(int _y = kUnsetPosition);

    /**
     * @brief Скорректировать настройки интерфейса в зависимости от того, как долго пользователь
     * ждёт соединения
     */
    void setWaitingTooLong(bool _isWaitingooLong);

    /**
     * @brief Показать/скрыть панель
     */
    void show();
    void hide();

    /**
     * @brief Скрывается ли тулбар в данный момент
     */
    bool isHiding() const;


    ConnectionStatusToolBar* q = nullptr;

    bool isWaitingTooLong = false;

    QAction* progressAction = nullptr;

    QParallelAnimationGroup statusAnimation;
    QVariantAnimation* startAngleAnimation = nullptr;
    QVariantAnimation* spanAngleAnimation = nullptr;

    QVariantAnimation positionAnimation;
};

ConnectionStatusToolBar::Implementation::Implementation(ConnectionStatusToolBar* _q)
    : q(_q)
    , progressAction(new QAction(q))
    , startAngleAnimation(new QVariantAnimation(q))
    , spanAngleAnimation(new QVariantAnimation(q))
{
    const int startAngle = 90;
    const int endAngle = -270;
    const int startSpanAngle = 0;
    const int endSpanAngle = -270;
    const int duration = 2000;
    const int firstPhaseDuration = duration / 4 * 3;
    const int secondPhaseDuration = duration / 4;
    auto initFirstPhase = [=] {
        startAngleAnimation->setStartValue(startAngle);
        startAngleAnimation->setEndValue(endAngle);
        startAngleAnimation->setDuration(firstPhaseDuration);

        spanAngleAnimation->setStartValue(startSpanAngle);
        spanAngleAnimation->setEndValue(endSpanAngle);
        spanAngleAnimation->setDuration(firstPhaseDuration);
    };
    auto initSecondPhase = [=] {
        startAngleAnimation->setStartValue(startAngle);
        startAngleAnimation->setEndValue(endAngle);
        startAngleAnimation->setDuration(secondPhaseDuration);

        spanAngleAnimation->setStartValue(endSpanAngle);
        spanAngleAnimation->setEndValue(startSpanAngle);
        spanAngleAnimation->setDuration(secondPhaseDuration);
    };

    initFirstPhase();

    connect(&statusAnimation, &QVariantAnimation::finished, startAngleAnimation,
            [this, endSpanAngle, initFirstPhase, initSecondPhase] {
                if (isWaitingTooLong) {
                    return;
                }

                // first phase
                if (spanAngleAnimation->startValue() == endSpanAngle) {
                    initFirstPhase();
                }
                // second phase
                else {
                    initSecondPhase();
                }

                if (q->isVisible()) {
                    statusAnimation.start();
                }
            });

    statusAnimation.addAnimation(startAngleAnimation);
    statusAnimation.addAnimation(spanAngleAnimation);

    positionAnimation.setEasingCurve(QEasingCurve::OutQuad);
    positionAnimation.setDuration(240);
}

void ConnectionStatusToolBar::Implementation::correctPosition(int _y)
{
    q->move(q->isLeftToRight()
                ? (q->parentWidget()->width() - q->width() - DesignSystem::layout().px24())
                : DesignSystem::layout().px24(),
            _y == kUnsetPosition
                ? (q->parentWidget()->height() - q->height() - DesignSystem::layout().px24())
                : _y);
}

void ConnectionStatusToolBar::Implementation::setWaitingTooLong(bool _isWaitingooLong)
{
    if (isWaitingTooLong == _isWaitingooLong) {
        return;
    }

    if (isWaitingTooLong) {
        statusAnimation.stop();
    } else {
        statusAnimation.start();
    }
    isWaitingTooLong = _isWaitingooLong;
    progressAction->setIconText(isWaitingTooLong ? u8"\U000F05AA" : "");
    q->setFlat(isWaitingTooLong);
    q->designSystemChangeEvent(nullptr);
}

void ConnectionStatusToolBar::Implementation::show()
{
    if (q->isVisible()) {
        return;
    }

    positionAnimation.setStartValue(q->parentWidget()->height());
    positionAnimation.setEndValue(q->parentWidget()->height() - q->height()
                                  - static_cast<int>(DesignSystem::layout().px24()));
    positionAnimation.start();
    q->show();
}

void ConnectionStatusToolBar::Implementation::hide()
{
    if (q->isHidden()) {
        return;
    }

    positionAnimation.setStartValue(q->parentWidget()->height() - q->height()
                                    - static_cast<int>(DesignSystem::layout().px24()));
    positionAnimation.setEndValue(q->parentWidget()->height());
    positionAnimation.start();
}

bool ConnectionStatusToolBar::Implementation::isHiding() const
{
    return positionAnimation.startValue().toInt() < positionAnimation.endValue().toInt();
}


// ****


ConnectionStatusToolBar::ConnectionStatusToolBar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    Q_ASSERT(_parent);
    _parent->installEventFilter(this);

    setFocusPolicy(Qt::NoFocus);

    addAction(d->progressAction);
    connect(d->progressAction, &QAction::triggered, this,
            &ConnectionStatusToolBar::checkConnectionPressed);

    connect(d->startAngleAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ConnectionStatusToolBar::update));
    connect(d->spanAngleAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ConnectionStatusToolBar::update));
    connect(&d->positionAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { d->correctPosition(_value.toInt()); });
    connect(&d->positionAnimation, &QVariantAnimation::finished, this, [this] {
        //
        // Если скрываем панель с экрана, то по завершении остановим анимацию прогресса и скроем
        // виджет полностью
        //
        if (d->isHiding()) {
            d->statusAnimation.stop();
            hide();
        }
    });

    hide();
}

ConnectionStatusToolBar::~ConnectionStatusToolBar() = default;

void ConnectionStatusToolBar::setConnectionAvailable(bool _available)
{
    if (_available) {
        d->setWaitingTooLong(false);
        d->hide();
        return;
    }

    d->statusAnimation.start();
    d->show();

    QTimer::singleShot(std::chrono::minutes{ 1 }, this, [this] {
        if (isVisible()) {
            d->setWaitingTooLong(true);
        }
    });
}

bool ConnectionStatusToolBar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parentWidget()) {
        if (_event->type() == QEvent::Resize) {
            if (d->positionAnimation.state() == QVariantAnimation::Running) {
                d->positionAnimation.pause();
                if (d->isHiding()) {
                    d->positionAnimation.setEndValue(parentWidget()->height());
                } else {
                    d->positionAnimation.setEndValue(parentWidget()->height() - height()
                                                     - DesignSystem::layout().px24());
                }
                d->positionAnimation.resume();
            } else {
                d->correctPosition();
            }
        } else if (_event->type() == QEvent::ChildAdded) {
            raise();
        }
    }

    return FloatingToolBar::eventFilter(_watched, _event);
}

void ConnectionStatusToolBar::paintEventPostprocess(QPainter& _painter)
{
    if (d->isWaitingTooLong || d->startAngleAnimation->state() != QVariantAnimation::Running
        || d->spanAngleAnimation->state() != QVariantAnimation::Running) {
        return;
    }

    QPen pen;
    pen.setWidthF(DesignSystem::layout().px4());
    pen.setCapStyle(Qt::SquareCap);
    pen.setCosmetic(true);
    pen.setColor(DesignSystem::color().error());
    _painter.setPen(pen);

    const auto circleRect
        = QRectF(rect()).marginsRemoved(DesignSystem::floatingToolBar().shadowMargins()
                                        + DesignSystem::floatingToolBar().margins());
    _painter.drawArc(circleRect, d->startAngleAnimation->currentValue().toInt() * 16,
                     d->spanAngleAnimation->currentValue().toInt() * 16);
}

void ConnectionStatusToolBar::updateTranslations()
{
    actions().constFirst()->setToolTip(tr("Connection with server lost. Automatic reconnection "
                                          "enabled. Press, to try to reconnect right now."));
}

void ConnectionStatusToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    setTextColor(ColorHelper::transparent(DesignSystem::color().onPrimary(),
                                          DesignSystem::inactiveTextOpacity()));
    setBackgroundColor(d->isWaitingTooLong ? Qt::transparent : DesignSystem::color().primary());

    raise();
    resize(sizeHint());
    d->correctPosition();
}

} // namespace Ui
