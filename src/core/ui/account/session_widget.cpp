#include "session_widget.h"

#include <domain/session_info.h>
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


    Domain::SessionInfo sessionInfo;

    H6Label* deviceName = nullptr;
    Body1Label* location = nullptr;
    IconsSmallLabel* lastUsedIcon = nullptr;
    Body1Label* lastUsed = nullptr;
    Button* terinateSession = nullptr;
    QVBoxLayout* layout = nullptr;
};

SessionWidget::Implementation::Implementation(QWidget* _parent)
    : deviceName(new H6Label(_parent))
    , location(new Body1Label(_parent))
    , lastUsedIcon(new IconsSmallLabel(_parent))
    , lastUsed(new Body1Label(_parent))
    , terinateSession(new Button(_parent))
    , layout(new QVBoxLayout)
{
    lastUsedIcon->setIcon(u8"\U000F05E0");
}


// ****


SessionWidget::SessionWidget(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    d->layout->setContentsMargins({});
    d->layout->setSpacing(0);
    d->layout->addWidget(d->deviceName);
    d->layout->addWidget(d->location);
    const auto lastUsedLayout = new QHBoxLayout;
    lastUsedLayout->setContentsMargins({});
    lastUsedLayout->setSpacing(0);
    lastUsedLayout->addWidget(d->lastUsedIcon, 0, Qt::AlignCenter);
    lastUsedLayout->addWidget(d->lastUsed, 1);
    d->layout->addLayout(lastUsedLayout);
    d->layout->addWidget(d->terinateSession, 0, Qt::AlignRight);
    setLayoutReimpl(d->layout);


    connect(d->terinateSession, &Button::clicked, this, [this] {
        if (d->sessionInfo.isCurrentDevice) {
            emit terminateOthersRequested();
        } else {
            emit terminateRequested();
        }
    });


    updateTranslations();
    designSystemChangeEvent(nullptr);
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

void SessionWidget::hideterminateButton()
{
    d->terinateSession->hide();
}

void SessionWidget::updateTranslations()
{
    if (d->sessionInfo.isCurrentDevice) {
        d->lastUsed->setText(tr("Current device"));
    }
    //
    // TODO: Сделать красиво - сегодня и вчера
    //
    else {
        const auto lastUsed = d->sessionInfo.lastUsed;
        if (QDateTime::currentSecsSinceEpoch() - lastUsed.toSecsSinceEpoch() < 180) {
            d->lastUsed->setText(tr("Online"));
        } else {
            //: Last active date (%1) and time (%2) of the user's device
            d->lastUsed->setText(
                tr("was active %1 at %2")
                    .arg(lastUsed.toString("dd.MM.yyyy"), lastUsed.toString("hh:mm")));
        }
    }
    d->terinateSession->setText(d->sessionInfo.isCurrentDevice ? tr("Terminate others")
                                                               : tr("Terminate"));
}

void SessionWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().background());

    const auto labelColor = ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                     DesignSystem::inactiveTextOpacity());
    for (auto label : std::vector<Widget*>{
             d->deviceName,
             d->location,
             d->lastUsed,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(labelColor);
    }
    d->deviceName->setTextColor(Ui::DesignSystem::color().onBackground());

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setRight(0);
    labelMargins.setBottom(0);
    d->deviceName->setContentsMargins(labelMargins);
    labelMargins.setTop(Ui::DesignSystem::layout().px8());
    d->location->setContentsMargins(labelMargins);
    labelMargins.setBottom(Ui::DesignSystem::layout().px24());
    if (d->sessionInfo.isCurrentDevice) {
        d->lastUsedIcon->setBackgroundColor(Ui::DesignSystem::color().background());
        d->lastUsedIcon->setTextColor(ColorHelper::transparent(
            Ui::DesignSystem::color().secondary(), Ui::DesignSystem::inactiveTextOpacity()));
        d->lastUsedIcon->setContentsMargins(labelMargins);
        labelMargins.setLeft(Ui::DesignSystem::layout().px8());
    }
    d->lastUsed->setContentsMargins(labelMargins);

    d->terinateSession->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->terinateSession->setTextColor(Ui::DesignSystem::color().secondary());

    d->layout->setContentsMargins(0, 0, Ui::DesignSystem::layout().px16(),
                                  Ui::DesignSystem::layout().px16());
}

} // namespace Ui
