#include "purchase_dialog_option.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>


namespace Ui {

class PurchaseDialogOption::Implementation
{
public:
    explicit Implementation(const Domain::PaymentOption& _option);


    bool isChecked = false;
    Domain::PaymentOption option;

    /**
     * @brief  Декорации при клике
     */
    ClickAnimation decorationAnimation;
};

PurchaseDialogOption::Implementation::Implementation(const Domain::PaymentOption& _option)
    : option(_option)
{
    decorationAnimation.setFast(false);
}


// ****


PurchaseDialogOption::PurchaseDialogOption(const Domain::PaymentOption& _option, QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(_option))
{

    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&PurchaseDialogOption::update));
}

PurchaseDialogOption::~PurchaseDialogOption() = default;

const Domain::PaymentOption& PurchaseDialogOption::paymentOption() const
{
    return d->option;
}

bool PurchaseDialogOption::isChecked() const
{
    return d->isChecked;
}

void PurchaseDialogOption::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    update();
}

QSize PurchaseDialogOption::sizeHint() const
{
    return QSize(10,
                 contentsMargins().top()
                     + (d->option.subscriptionType == Domain::SubscriptionType::ProLifetime
                            ? Ui::DesignSystem::layout().px(82)
                            : Ui::DesignSystem::layout().px(124))
                     + contentsMargins().bottom());
}

void PurchaseDialogOption::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Заливаем подложку и рисуем рамку
    //
    painter.setPen(
        d->isChecked
            ? QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px2())
            : (underMouse()
                   ? QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px())
                   : QPen(ColorHelper::transparent(textColor(),
                                                   Ui::DesignSystem::elevationEndOpacity()),
                          Ui::DesignSystem::layout().px())));
    painter.setBrush(underMouse() ? ColorHelper::transparent(
                         textColor(), Ui::DesignSystem::elevationEndOpacity())
                                  : backgroundColor());
    painter.drawRoundedRect(contentsRect(), Ui::DesignSystem::card().borderRadius(),
                            Ui::DesignSystem::card().borderRadius());

    const auto subscriptionTypeTitle
        = (d->option.subscriptionType == Domain::SubscriptionType::ProMonthly
           || d->option.subscriptionType == Domain::SubscriptionType::ProLifetime)
        ? "PRO"
        : "TEAM";
    const auto paymentMethodsTitle = tr("Pay with:");
    const auto regularPrice = QString("$%1").arg(d->option.amount / 100.0, 0, 'f', 2);
    const auto totalPrice = QString("$%1").arg(d->option.totalAmount / 100.0, 0, 'f', 2);
    auto paymentMethodsText
        = [](int _price) { return _price >= 2000 ? u8" \U000F019B \U000F0813" : u8" \U000F019B"; };

    //
    // Лайфтайм версия рисуется на всю ширину
    //
    if (d->option.duration == Domain::PaymentDuration::Lifetime) {
        auto textRect = contentsRect().adjusted(
            Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16(),
            -Ui::DesignSystem::layout().px24(), -Ui::DesignSystem::layout().px16());
        //
        // ... заголовок
        //
        painter.setPen(textColor());
        painter.setFont(Ui::DesignSystem::font().body1());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop,
                         tr("%1 lifetime").arg(subscriptionTypeTitle));

        //
        // ... цена без скидки
        //
        painter.setPen(Ui::DesignSystem::color().accent());
        painter.setFont(Ui::DesignSystem::font().button());
        if (d->option.amount != d->option.totalAmount) {
            painter.setPen(
                ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity()));
            auto font = painter.font();
            font.setStrikeOut(true);
            painter.setFont(font);
        }
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignTop, regularPrice);
        //
        // ... цена со скидкой
        //
        if (d->option.amount != d->option.totalAmount) {
            painter.setPen(Ui::DesignSystem::color().accent());
            painter.setFont(Ui::DesignSystem::font().button());
            auto totalPriceRect = textRect.adjusted(
                0, TextHelper::fineLineSpacing(painter.font()) + Ui::DesignSystem::layout().px8(),
                0, 0);
            painter.drawText(totalPriceRect, Qt::AlignRight | Qt::AlignTop, totalPrice);
        }
        //
        // ... способы оплаты
        //
        painter.setPen(
            ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity()));
        painter.setFont(Ui::DesignSystem::font().caption());
        painter.drawText(textRect.adjusted(0, 0, 0, -Ui::DesignSystem::layout().px4()),
                         Qt::AlignLeft | Qt::AlignBottom, paymentMethodsTitle);
        //
        textRect.setLeft(textRect.left()
                         + TextHelper::fineTextWidthF(paymentMethodsTitle, painter.font()));
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        painter.drawText(textRect.adjusted(0, 0, 0, Ui::DesignSystem::layout().px2()),
                         Qt::AlignLeft | Qt::AlignBottom,
                         paymentMethodsText(d->option.totalAmount));
    }
    //
    // Остальные версии рисуются в несколько строк
    // - название 24
    // - стоимость месяца 16
    // - полная стоимость 24
    //
    else {
        auto textRect = QRectF(contentsMargins().left() + Ui::DesignSystem::layout().px24(),
                               contentsMargins().top() + Ui::DesignSystem::layout().px16(),
                               contentsRect().width(), Ui::DesignSystem::layout().px24());
        const auto months = static_cast<int>(d->option.duration);
        const auto isOneMonth = months == 1;
        //
        // ... заголовок
        //
        painter.setPen(textColor());
        painter.setFont(Ui::DesignSystem::font().body1());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                         tr("%1 for %2").arg(subscriptionTypeTitle, tr("%n month(s)", "", months)));
        //
        // ... месячная стоимость
        //
        if (!isOneMonth) {
            textRect.moveTop(textRect.bottom() + Ui::DesignSystem::layout().px4());
            textRect.setHeight(Ui::DesignSystem::layout().px16());
            painter.setPen(
                ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity()));
            painter.setFont(Ui::DesignSystem::font().caption());
            painter.drawText(
                textRect, Qt::AlignLeft | Qt::AlignVCenter,
                tr("$%1 per month")
                    .arg(d->option.totalAmount / 100.0 / static_cast<qreal>(d->option.duration), 0,
                         'f', 2));
        }
        //
        // ... стоимость без скидки
        //
        textRect.moveTop(textRect.bottom() + Ui::DesignSystem::layout().px2());
        textRect.setHeight(Ui::DesignSystem::layout().px24());
        painter.setPen(Ui::DesignSystem::color().accent());
        painter.setFont(Ui::DesignSystem::font().button());
        if (d->option.amount != d->option.totalAmount) {
            painter.setPen(
                ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity()));
            auto font = painter.font();
            font.setStrikeOut(true);
            painter.setFont(font);
        }
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, regularPrice);
        //
        // ... стоимость со скидкой
        //
        if (d->option.amount != d->option.totalAmount) {
            auto totalPriceRect = textRect;
            totalPriceRect.setLeft(textRect.left()
                                   + TextHelper::fineTextWidthF(regularPrice, painter.font())
                                   + Ui::DesignSystem::layout().px8());
            painter.setPen(Ui::DesignSystem::color().accent());
            painter.setFont(Ui::DesignSystem::font().button());
            painter.drawText(totalPriceRect, Qt::AlignLeft | Qt::AlignVCenter, totalPrice);
        }
        //
        // ... если это была цена одного месяца, то нужно ещё немного сместить, т.к. строка месячной
        // стоимости не заполнялась
        //
        if (isOneMonth) {
            textRect.moveTop(textRect.top() + Ui::DesignSystem::layout().px16()
                             + Ui::DesignSystem::layout().px4());
        }
        //
        // ... способы оплаты
        //
        textRect.moveTop(textRect.bottom() + Ui::DesignSystem::layout().px2());
        painter.setPen(
            ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity()));
        painter.setFont(Ui::DesignSystem::font().caption());
        painter.drawText(textRect.adjusted(0, 0, 0, -Ui::DesignSystem::layout().px4()),
                         Qt::AlignLeft | Qt::AlignBottom, paymentMethodsTitle);
        //
        textRect.setLeft(textRect.left()
                         + TextHelper::fineTextWidthF(paymentMethodsTitle, painter.font()));
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        painter.drawText(textRect.adjusted(0, 0, 0, Ui::DesignSystem::layout().px2()),
                         Qt::AlignLeft | Qt::AlignBottom,
                         paymentMethodsText(d->option.totalAmount));
    }

    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().accent());
        painter.setClipRect(d->decorationAnimation.clipRect());
        painter.setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        painter.drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
        painter.setClipRect(QRectF(), Qt::NoClip);
        painter.setOpacity(1.0);
    }
}

void PurchaseDialogOption::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    if (!contentsRect().contains(_event->pos())) {
        return;
    }

    d->decorationAnimation.setClickPosition(_event->pos());
    d->decorationAnimation.setClipRect(contentsRect());
    d->decorationAnimation.setRadiusInterval(0.0, contentsRect().width());
    d->decorationAnimation.start();
}

void PurchaseDialogOption::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!contentsRect().contains(_event->pos())) {
        return;
    }

    if (d->isChecked) {
        return;
    }

    d->isChecked = true;
    emit checkedChanged(d->isChecked);
}

} // namespace Ui
