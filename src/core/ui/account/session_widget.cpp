#include "session_widget.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>

#include <NetworkRequestLoader.h>


namespace Ui {

class SessionWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    bool isOnline() const;


    Domain::SessionInfo sessionInfo;

    H6Label* deviceName = nullptr;
    Body1Label* location = nullptr;
    IconsSmallLabel* lastUsedIcon = nullptr;
    Body2Label* lastUsed = nullptr;
    Button* terminateSession = nullptr;
    QVBoxLayout* layout = nullptr;
};

SessionWidget::Implementation::Implementation(QWidget* _parent)
    : deviceName(new H6Label(_parent))
    , location(new Body1Label(_parent))
    , lastUsedIcon(new IconsSmallLabel(_parent))
    , lastUsed(new Body2Label(_parent))
    , terminateSession(new Button(_parent))
    , layout(new QVBoxLayout)
{
    lastUsedIcon->setIcon(u8"\U000F05E0");

    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(deviceName);
    layout->addWidget(location);
    const auto lastUsedLayout = new QHBoxLayout;
    lastUsedLayout->setContentsMargins({});
    lastUsedLayout->setSpacing(0);
    lastUsedLayout->addWidget(lastUsedIcon, 0, Qt::AlignCenter);
    lastUsedLayout->addWidget(lastUsed, 1);
    layout->addLayout(lastUsedLayout);
    layout->addWidget(terminateSession, 0, Qt::AlignRight);
}

bool SessionWidget::Implementation::isOnline() const
{
    const auto lastUsed = sessionInfo.lastUsed;
    return QDateTime::currentSecsSinceEpoch() - lastUsed.toSecsSinceEpoch() < 180;
}


// ****


SessionWidget::SessionWidget(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    setContentLayout(d->layout);

    connect(d->terminateSession, &Button::clicked, this, [this] {
        if (d->sessionInfo.isCurrentDevice) {
            emit terminateOthersRequested();
        } else {
            emit terminateRequested();
        }
    });
}

SessionWidget::~SessionWidget() = default;

Domain::SessionInfo SessionWidget::sessionInfo() const
{
    return d->sessionInfo;
}

void SessionWidget::setSessionInfo(const Domain::SessionInfo& _sessionInfo)
{
    d->sessionInfo = _sessionInfo;

    d->deviceName->setText(d->sessionInfo.deviceName);
    d->lastUsedIcon->setVisible(d->sessionInfo.isCurrentDevice);

    const auto ipToLocationUrl
        = QString("https://reallyfreegeoip.org/json/%1").arg(d->sessionInfo.location);
    NetworkRequestLoader::loadAsync(ipToLocationUrl, this, [this](const QByteArray& _data) {
        const auto json = QJsonDocument::fromJson(_data).object();
        const auto country = json["country_name"].toString();
        const auto region = json["region_name"].toString();
        const auto city = json["city"].toString();
        QString location = country + ", " + region;
        if (region != city) {
            location += ", " + city;
        }
        if (country.isEmpty()) {
            location = "Wizard's world";
        }
        d->location->setText(location);
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void SessionWidget::hideTerminateButton()
{
    d->terminateSession->hide();
}

void SessionWidget::updateTranslations()
{
    if (d->sessionInfo.isCurrentDevice) {
        d->lastUsed->setText(tr("Current device"));
    } else {
        if (d->isOnline()) {
            d->lastUsed->setText(tr("Online"));
        } else {
            const auto lastUsed = d->sessionInfo.lastUsed;
            QString lastUsedDate;
            const auto today = QDate::currentDate();
            if (lastUsed.date() == today) {
                lastUsedDate = tr("today");
            } else if (lastUsed.date().daysTo(today) == 1) {
                lastUsedDate = tr("yesterday");
            } else {
                lastUsedDate = lastUsed.toString("dd.MM.yyyy");
            }

            //: Last active date (%1) and time (%2) of the user's device
            d->lastUsed->setText(
                tr("was active %1 at %2").arg(lastUsedDate, lastUsed.toString("hh:mm")));
        }
    }
    d->terminateSession->setText(d->sessionInfo.isCurrentDevice ? tr("Terminate others")
                                                               : tr("Terminate"));
}

void SessionWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().background());

    const auto labelColor = ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                     DesignSystem::inactiveTextOpacity());
    for (auto label : std::vector<Widget*>{
             d->deviceName,
             d->location,
             d->lastUsed,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(labelColor);
    }
    d->deviceName->setTextColor(DesignSystem::color().onBackground());

    auto labelMargins = DesignSystem::label().margins().toMargins();
    labelMargins.setRight(0);
    labelMargins.setBottom(0);
    d->deviceName->setContentsMargins(labelMargins);
    labelMargins.setTop(DesignSystem::layout().px8());
    d->location->setContentsMargins(labelMargins);
    labelMargins.setBottom(DesignSystem::layout().px24());
    if (d->sessionInfo.isCurrentDevice) {
        d->lastUsedIcon->setBackgroundColor(DesignSystem::color().background());
        d->lastUsedIcon->setTextColor(ColorHelper::transparent(
            DesignSystem::color().accent(), DesignSystem::inactiveTextOpacity()));
        d->lastUsedIcon->setContentsMargins(labelMargins);
        labelMargins.setLeft(DesignSystem::layout().px8());
    }
    d->lastUsed->setContentsMargins(labelMargins);
    d->lastUsed->setTextColor(d->isOnline() ? DesignSystem::color().accent() : labelColor);

    d->terminateSession->setBackgroundColor(DesignSystem::color().accent());
    d->terminateSession->setTextColor(DesignSystem::color().accent());

    d->layout->setContentsMargins(0, 0, DesignSystem::layout().px16(),
                                  DesignSystem::layout().px16());
}

} // namespace Ui
