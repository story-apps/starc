#include "credits_view.h"

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

class CreditsView::Implementation
{
public:
    Implementation(CreditsView* _q, const Domain::Notification& _notification);

    /**
     * @brief Настроить представление и тексты в соответствии с уведомлением
     */
    void setupView();


    CreditsView* q = nullptr;

    Domain::Notification notification;

    IconsMidLabel* avatarLabel = nullptr;
    CaptionLabel* dateTimeLabel = nullptr;
    Subtitle2Label* titleLabel = nullptr;
    Body1Label* bodyLabel = nullptr;
};

CreditsView::Implementation::Implementation(CreditsView* _q,
                                            const Domain::Notification& _notification)
    : q(_q)
    , notification(_notification)
    , avatarLabel(new IconsMidLabel(_q))
    , dateTimeLabel(new CaptionLabel(_q))
    , titleLabel(new Subtitle2Label(_q))
    , bodyLabel(new Body1Label(_q))
{
    avatarLabel->setIcon(u8"\U000F133D");
    avatarLabel->setDecorationVisible(true);
}

void CreditsView::Implementation::setupView()
{
    dateTimeLabel->setText(notification.dateTime.toLocalTime().toString("dd.MM.yyyy hh:mm"));
    avatarLabel->setTextColor(Ui::DesignSystem::color().accent());
    const auto json = QJsonDocument::fromJson(notification.notification.toUtf8()).object();
    const auto credits = json.value("credits").toString().toInt();
    titleLabel->setText(tr("Credits added"));
    bodyLabel->setText(tr("%n credits were added to your balance.", nullptr, credits));
}

CreditsView::CreditsView(QWidget* _parent, const Domain::Notification& _notification)
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
    setLayout(layout);
}

CreditsView::~CreditsView() = default;

void CreditsView::updateTranslations()
{
    d->setupView();
}

void CreditsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
}

} // namespace Ui
