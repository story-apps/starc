#include "purchase_dialog_option_widget.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>


namespace Ui {

class PurchaseDialogOptionWidget::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Рисовать ли опцию на всю ширину
     */
    bool isFullWidth() const;


    bool isChecked = false;
    Domain::PaymentOption option;
    bool isWide = false;
    bool showTotal = false;

    /**
     * @brief  Декорации при клике
     */
    ClickAnimation decorationAnimation;
};

PurchaseDialogOptionWidget::Implementation::Implementation()
{
    decorationAnimation.setFast(false);
}

bool PurchaseDialogOptionWidget::Implementation::isFullWidth() const
{
    return option.duration == Domain::PaymentDuration::Lifetime || isWide;
}


// ****


PurchaseDialogOptionWidget::PurchaseDialogOptionWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&PurchaseDialogOptionWidget::update));
}

PurchaseDialogOptionWidget::~PurchaseDialogOptionWidget() = default;

void PurchaseDialogOptionWidget::setPaymentOption(const Domain::PaymentOption& _paymentOption)
{
    d->option = _paymentOption;

    updateGeometry();
    update();
}

const Domain::PaymentOption& PurchaseDialogOptionWidget::paymentOption() const
{
    return d->option;
}

void PurchaseDialogOptionWidget::setWide(bool _isWide)
{
    if (d->isWide == _isWide) {
        return;
    }

    d->isWide = _isWide;
    update();
}

void PurchaseDialogOptionWidget::setShowTotal(bool _showTotal)
{
    if (d->showTotal == _showTotal) {
        return;
    }

    d->showTotal = _showTotal;
    update();
}

bool PurchaseDialogOptionWidget::isChecked() const
{
    return d->isChecked;
}

void PurchaseDialogOptionWidget::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    update();
}

QSize PurchaseDialogOptionWidget::sizeHint() const
{
    return QSize(
        10,
        contentsMargins().top()
            + (d->isFullWidth() ? DesignSystem::layout().px(82) : DesignSystem::layout().px(124))
            + contentsMargins().bottom());
}

void PurchaseDialogOptionWidget::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Заливаем подложку и рисуем рамку
    //
    const auto isInteractive = isEnabled() && underMouse();
    painter.setPen(d->isChecked
                       ? QPen(DesignSystem::color().accent(), DesignSystem::layout().px2())
                       : (isInteractive
                              ? QPen(DesignSystem::color().accent(), DesignSystem::layout().px())
                              : QPen(ColorHelper::transparent(textColor(),
                                                              DesignSystem::elevationEndOpacity()),
                                     DesignSystem::layout().px())));
    painter.setBrush(
        isInteractive ? ColorHelper::transparent(textColor(), DesignSystem::elevationEndOpacity())
                      : backgroundColor());
    painter.drawRoundedRect(contentsRect(), DesignSystem::card().borderRadius(),
                            DesignSystem::card().borderRadius());

    const auto subscriptionTypeTitle
        = (d->option.subscriptionType == Domain::SubscriptionType::ProMonthly
           || d->option.subscriptionType == Domain::SubscriptionType::ProLifetime)
        ? "PRO"
        : "CLOUD";
    const auto paymentMethodsTitle = tr("Pay with:");
    const auto regularPrice = QString("$%1").arg(d->option.amount / 100.0, 0, 'f', 2);
    const auto basePrice = QString("$%1").arg(d->option.baseAmount() / 100.0, 0, 'f', 2);
    const auto totalPrice = QString("$%1").arg(d->option.totalAmount / 100.0, 0, 'f', 2);
    auto paymentMethodsText
        = [](int _price) { return _price >= 2000 ? u8" \U000F019B" : u8" \U000F019B"; };

    //
    // Вариант, когда рисуем опцию на всю ширину
    // NOTE: Это может быть только лайфтайм, или другая опция в диалоге покупки подписки в подарок,
    //       поэтому тут так по-дурацки сделана работа с ценами (на лайфтайм базовой скидки быть не
    //       может, а когда мы берём в подарок то для опции покупки сразу показывается финальная
    //       цена)
    //
    if (d->isFullWidth()) {
        auto textRect = contentsRect().adjusted(
            DesignSystem::layout().px24(), DesignSystem::layout().px16(),
            -DesignSystem::layout().px24(), -DesignSystem::layout().px16());
        //
        // ... заголовок
        //
        painter.setPen(textColor());
        painter.setFont(DesignSystem::font().body1());
        const auto title = d->option.duration == Domain::PaymentDuration::Lifetime
            ? tr("%1 lifetime").arg(subscriptionTypeTitle)
            : (d->option.type == Domain::PaymentType::Subscription
                   ? tr("%1 for %2")
                         .arg(subscriptionTypeTitle,
                              tr("%n month(s)", "", static_cast<int>(d->option.duration)))
                   : tr("%1 credits").arg(d->option.credits));
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, title);
        //
        // ... цена
        //
        painter.setPen(DesignSystem::color().accent());
        painter.setFont(DesignSystem::font().button());
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignTop,
                         d->showTotal ? totalPrice : regularPrice);
        //
        // ... способы оплаты
        //
        painter.setPen(ColorHelper::transparent(textColor(), DesignSystem::inactiveTextOpacity()));
        painter.setFont(DesignSystem::font().caption());
        painter.drawText(textRect.adjusted(0, 0, 0, -DesignSystem::layout().px4()),
                         Qt::AlignLeft | Qt::AlignBottom, paymentMethodsTitle);
        //
        textRect.setLeft(textRect.left()
                         + TextHelper::fineTextWidthF(paymentMethodsTitle, painter.font()));
        painter.setFont(DesignSystem::font().iconsMid());
        painter.drawText(textRect.adjusted(0, 0, 0, DesignSystem::layout().px2()),
                         Qt::AlignLeft | Qt::AlignBottom,
                         paymentMethodsText(d->option.totalAmount));
    }
    //
    // В противном случае опция рисуются в несколько строк
    // - название 24
    // - стоимость месяца 16
    // - полная стоимость 24
    //
    else {
        auto textRect = QRectF(contentsMargins().left() + DesignSystem::layout().px24(),
                               contentsMargins().top() + DesignSystem::layout().px16(),
                               contentsRect().width(), DesignSystem::layout().px24());
        const auto months = static_cast<int>(d->option.duration);
        //
        // ... заголовок
        //
        painter.setPen(textColor());
        painter.setFont(DesignSystem::font().body1());
        const auto title = d->option.type == Domain::PaymentType::Subscription
            ? tr("%1 for %2").arg(subscriptionTypeTitle, tr("%n month(s)", "", months))
            : tr("%1 credits").arg(d->option.credits);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        //
        // ... размер скидки
        //
        if (d->option.baseDiscount > 0) {
            const auto backgroundColor = ColorHelper::transparent(
                DesignSystem::color().accent(), DesignSystem::inactiveTextOpacity());
            painter.setPen(backgroundColor);
            painter.setBrush(backgroundColor);
            const auto discountRect
                = QRectF(contentsRect().right() - DesignSystem::layout().px(54),
                         DesignSystem::layout().px(20), DesignSystem::layout().px(38),
                         DesignSystem::layout().px(20));
            painter.drawRoundedRect(discountRect, DesignSystem::layout().px4(),
                                    DesignSystem::layout().px4());

            painter.setPen(DesignSystem::color().onAccent());
            painter.setBrush(Qt::NoBrush);
            painter.setFont(DesignSystem::font().caption());
            painter.drawText(discountRect, Qt::AlignCenter,
                             QString("-%1%").arg(static_cast<int>(d->option.baseDiscount)));
        }
        //
        // ... добавляем месячную стоимость
        //
        if (d->option.type == Domain::PaymentType::Subscription && months > 1) {
            textRect.moveTop(textRect.bottom() + DesignSystem::layout().px4());
            textRect.setHeight(DesignSystem::layout().px16());
            painter.setPen(
                ColorHelper::transparent(textColor(), DesignSystem::inactiveTextOpacity()));
            painter.setFont(DesignSystem::font().caption());
            painter.drawText(
                textRect, Qt::AlignLeft | Qt::AlignVCenter,
                tr("$%1 per month")
                    .arg(d->option.baseAmount() / 100.0 / static_cast<qreal>(d->option.duration), 0,
                         'f', 2));
        }
        //
        // ... стоимость без скидки
        //
        textRect.moveTop(textRect.bottom() + DesignSystem::layout().px2());
        textRect.setHeight(DesignSystem::layout().px24());
        painter.setPen(DesignSystem::color().accent());
        painter.setFont(DesignSystem::font().button());
        if (d->option.amount != d->option.baseAmount()) {
            painter.setPen(
                ColorHelper::transparent(textColor(), DesignSystem::disabledTextOpacity()));
            auto font = painter.font();
            font.setStrikeOut(true);
            painter.setFont(font);
        }
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, regularPrice);
        //
        // ... стоимость со скидкой
        //
        if (d->option.amount != d->option.baseAmount()) {
            auto totalPriceRect = textRect;
            totalPriceRect.setLeft(textRect.left()
                                   + TextHelper::fineTextWidthF(regularPrice, painter.font())
                                   + DesignSystem::layout().px8());
            painter.setPen(DesignSystem::color().accent());
            painter.setFont(DesignSystem::font().button());
            painter.drawText(totalPriceRect, Qt::AlignLeft | Qt::AlignVCenter, basePrice);
        }
        //
        // ... если это была цена одного месяца, то нужно ещё немного сместить, т.к. строка месячной
        // стоимости не заполнялась
        //
        if (months == 1 || d->option.type == Domain::PaymentType::Credits) {
            textRect.moveTop(textRect.top() + DesignSystem::layout().px16()
                             + DesignSystem::layout().px4());
        }
        //
        // ... способы оплаты
        //
        textRect.moveTop(textRect.bottom() + DesignSystem::layout().px2());
        painter.setPen(ColorHelper::transparent(textColor(), DesignSystem::inactiveTextOpacity()));
        painter.setFont(DesignSystem::font().caption());
        painter.drawText(textRect.adjusted(0, 0, 0, -DesignSystem::layout().px4()),
                         Qt::AlignLeft | Qt::AlignBottom, paymentMethodsTitle);
        //
        textRect.setLeft(textRect.left()
                         + TextHelper::fineTextWidthF(paymentMethodsTitle, painter.font()));
        painter.setFont(DesignSystem::font().iconsMid());
        painter.drawText(textRect.adjusted(0, 0, 0, DesignSystem::layout().px2()),
                         Qt::AlignLeft | Qt::AlignBottom,
                         paymentMethodsText(d->option.baseAmount()));
    }

    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(DesignSystem::color().accent());
        painter.setClipRect(d->decorationAnimation.clipRect());
        painter.setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        painter.drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
        painter.setClipRect(QRectF(), Qt::NoClip);
        painter.setOpacity(1.0);
    }
}

void PurchaseDialogOptionWidget::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    if (!isEnabled() || !contentsRect().contains(_event->pos())) {
        return;
    }

    d->decorationAnimation.setClickPosition(_event->pos());
    d->decorationAnimation.setClipRect(contentsRect());
    d->decorationAnimation.setRadiusInterval(0.0, contentsRect().width());
    d->decorationAnimation.start();
}

void PurchaseDialogOptionWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!isEnabled() || !contentsRect().contains(_event->pos())) {
        return;
    }

    if (d->isChecked) {
        return;
    }

    d->isChecked = true;
    emit checkedChanged(d->isChecked);
}

} // namespace Ui
