#include "subscription_view.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>


namespace Ui {

class SubscriptionView::Implementation
{
public:
    Implementation(SubscriptionView* _q, const Domain::Notification& _notification);

    /**
     * @brief Настроить представление и тексты в соответствии с уведомлением
     */
    void setupView();


    SubscriptionView* q = nullptr;

    Domain::Notification notification;

    IconsMidLabel* avatarLabel = nullptr;
    CaptionLabel* dateTimeLabel = nullptr;
    Subtitle2Label* titleLabel = nullptr;
    Body1Label* bodyLabel = nullptr;
    Button* renewButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

SubscriptionView::Implementation::Implementation(SubscriptionView* _q,
                                                 const Domain::Notification& _notification)
    : q(_q)
    , notification(_notification)
    , avatarLabel(new IconsMidLabel(_q))
    , dateTimeLabel(new CaptionLabel(_q))
    , titleLabel(new Subtitle2Label(_q))
    , bodyLabel(new Body1Label(_q))
    , renewButton(new Button(_q))
    , buttonsLayout(new QHBoxLayout)
{
    avatarLabel->setIcon(notification.type == Domain::NotificationType::ProSubscriptionEnds
                             ? u8"\U000F18BD"
                             : u8"\U000F0163");
    avatarLabel->setDecorationVisible(true);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(renewButton);
    buttonsLayout->addStretch();
}

void SubscriptionView::Implementation::setupView()
{
    dateTimeLabel->setText(notification.dateTime.toLocalTime().toString("dd.MM.yyyy hh:mm"));
    const auto json = QJsonDocument::fromJson(notification.notification.toUtf8()).object();
    const auto endDateTime
        = QDateTime::fromString(json.value("end_date").toString(), Qt::ISODateWithMs);
    const int daysLeft = notification.dateTime.daysTo(endDateTime);
    avatarLabel->setTextColor(daysLeft <= 0 ? Ui::DesignSystem::color().error()
                                            : Ui::DesignSystem::color().accent());
    QString title;
    QString body;
    bool isButtonVisible = true;

    //
    // Удаление проектов
    //
    if (daysLeft < -1) {
        title = tr("Cloud projects removal");
        body = tr("Your Story Architect cloud projects will be removed tomorrow if you don't "
                  "renew CLOUD subscription.");
    }
    //
    // Подписка закончилась
    //
    else if (daysLeft == -1) {
        title = tr("Subscription ended");
        body = notification.type == Domain::NotificationType::ProSubscriptionEnds
            ? tr("Your PRO version subscription is expired. Account is switched to the FREE "
                 "version.")
            : tr("Your CLOUD version subscription is expired. Your cloud projects will be stored "
                 "for 30 days and then removed if you don't reactivate CLOUD subscription.");
    }
    //
    // Последний день подписки
    //
    else if (daysLeft == 0) {
        title = tr("Subscription ends");
        body = notification.type == Domain::NotificationType::ProSubscriptionEnds
            ? tr("Your PRO version subscription expires today.")
            : tr("Your CLOUD version subscription expires today.");
    }
    //
    // Подписка скоро закончится
    //
    else if (daysLeft < 10) {
        title = tr("Subscription ends");
        body = notification.type == Domain::NotificationType::ProSubscriptionEnds
            ? tr("Your PRO version subscription expires in %n day(s).", "", std::max(1, daysLeft))
            : tr("Your CLOUD version subscription expires in %n day(s).", "",
                 std::max(1, daysLeft));
    }
    //
    // Подписка продлена на период
    //
    else if (daysLeft < 1000) {
        title = tr("Subscription renewed");
        body = notification.type == Domain::NotificationType::ProSubscriptionEnds
            ? tr("Your PRO version subscription active until %1.")
                  .arg(endDateTime.toString("dd.MM.yyyy"))
            : tr("Your CLOUD version subscription active until %1.")
                  .arg(endDateTime.toString("dd.MM.yyyy"));
        isButtonVisible = false;
    }
    //
    // Подписка продлена навсегда
    //
    else {
        title = tr("Subscription renewed");
        body = notification.type == Domain::NotificationType::ProSubscriptionEnds
            ? tr("Your PRO version lifetime subscription activated.")
            : tr("Your CLOUD version lifetime subscription activated.");
        isButtonVisible = false;
    }

    titleLabel->setText(title);
    bodyLabel->setText(body);
    renewButton->setText(tr("Activate"));
    renewButton->setVisible(isButtonVisible);
}

SubscriptionView::SubscriptionView(QWidget* _parent, const Domain::Notification& _notification)
    : Widget(_parent)
    , d(new Implementation(this, _notification))
{
    auto titleLayout = new QVBoxLayout;
    titleLayout->setContentsMargins({});
    titleLayout->setSpacing(0);
    titleLayout->addWidget(d->dateTimeLabel);
    titleLayout->addWidget(d->titleLabel);

    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->avatarLabel);
    topLayout->addLayout(titleLayout);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->bodyLabel);
    layout->addLayout(d->buttonsLayout);
    setLayout(layout);

    connect(d->renewButton, &Button::clicked, this, [this] {
        switch (d->notification.type) {
        case Domain::NotificationType::ProSubscriptionEnds: {
            emit renewProPressed();
            break;
        }

        case Domain::NotificationType::TeamSubscriptionEnds: {
            emit renewTeamPressed();
            break;
        }

        default: {
            break;
        }
        }
    });
}

SubscriptionView::~SubscriptionView() = default;

void SubscriptionView::updateTranslations()
{
    d->setupView();
}

void SubscriptionView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    for (auto widget : std::vector<Widget*>{
             this,
             d->avatarLabel,
             d->dateTimeLabel,
             d->titleLabel,
             d->bodyLabel,
         }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().primary());
    }

    d->avatarLabel->setFixedSize(Ui::DesignSystem::layout().px(72),
                                 Ui::DesignSystem::layout().px(72));
    const auto leftMargin
        = isLeftToRight() ? Ui::DesignSystem::layout().px24() : Ui::DesignSystem::layout().px12();
    const auto rightMargin
        = isLeftToRight() ? Ui::DesignSystem::layout().px12() : Ui::DesignSystem::layout().px24();
    d->avatarLabel->setContentsMargins(leftMargin, Ui::DesignSystem::layout().px24(), rightMargin,
                                       Ui::DesignSystem::layout().px12());
    d->dateTimeLabel->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onPrimary(), Ui::DesignSystem::disabledTextOpacity()));
    d->dateTimeLabel->setAlignment(isLeftToRight() ? Qt::AlignLeft : Qt::AlignRight);
    d->dateTimeLabel->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);
    d->titleLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->titleLabel->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());
    d->bodyLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->bodyLabel->setContentsMargins(leftMargin, 0, rightMargin, Ui::DesignSystem::layout().px12());
    d->renewButton->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->renewButton->setTextColor(Ui::DesignSystem::color().accent());
    d->renewButton->setContentsMargins(0, Ui::DesignSystem::layout().px(6), 0,
                                       Ui::DesignSystem::layout().px4());

    d->buttonsLayout->setContentsMargins(
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px4(),
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px16());
}

} // namespace Ui
