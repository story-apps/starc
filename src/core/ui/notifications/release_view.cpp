#include "release_view.h"

#include <domain/notification.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/progress_bar/progress_bar.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>


namespace Ui {

class ReleaseView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Domain::Notification notification;

    ImageLabel* avatarLabel = nullptr;
    CaptionLabel* dateTimeLabel = nullptr;
    Subtitle2Label* titleLabel = nullptr;
    Body1Label* bodyLabel = nullptr;
    Subtitle2LinkLabel* readMoreLink = nullptr;
    Button* installButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

ReleaseView::Implementation::Implementation(QWidget* _parent)
    : avatarLabel(new ImageLabel(_parent))
    , dateTimeLabel(new CaptionLabel(_parent))
    , titleLabel(new Subtitle2Label(_parent))
    , bodyLabel(new Body1Label(_parent))
    , readMoreLink(new Subtitle2LinkLabel(_parent))
    , installButton(new Button(_parent))
    , buttonsLayout(new QHBoxLayout)
{
    avatarLabel->setImage(QPixmap(":/images/logo"));
}


// ****


ReleaseView::ReleaseView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
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

    d->buttonsLayout->setContentsMargins({});
    d->buttonsLayout->setSpacing(0);
    d->buttonsLayout->addWidget(d->installButton);
    d->buttonsLayout->addStretch();

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->bodyLabel);
    layout->addWidget(d->readMoreLink);
    layout->addLayout(d->buttonsLayout);
    setLayout(layout);
}

ReleaseView::~ReleaseView() = default;

void ReleaseView::setNotification(const Domain::Notification& _notification)
{
    d->notification = _notification;

    updateTranslations();
}

void ReleaseView::updateTranslations()
{
    const auto json = QJsonDocument::fromJson(d->notification.notification.toUtf8()).object();
    d->dateTimeLabel->setText(d->notification.dateTime.toString("dd.MM.yyyy"));
    d->titleLabel->setText(tr("Version %1 published").arg(json.value("version").toString()));
    d->bodyLabel->setText(json.value("info").toString());
    d->bodyLabel->setVisible(!d->bodyLabel->text().isEmpty());
    d->readMoreLink->setText(tr("Read more"));
    d->readMoreLink->setLink(json.value("read_more_link").toString());
    d->installButton->setText(tr("Install"));
}

void ReleaseView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    for (auto widget : std::vector<Widget*>{
             this,
             d->avatarLabel,
             d->dateTimeLabel,
             d->titleLabel,
             d->bodyLabel,
             d->readMoreLink,
         }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().primary());
    }

    d->avatarLabel->setFixedSize(Ui::DesignSystem::layout().px(72),
                                 Ui::DesignSystem::layout().px(72));
    d->avatarLabel->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12());
    d->dateTimeLabel->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onPrimary(), Ui::DesignSystem::disabledTextOpacity()));
    d->dateTimeLabel->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);
    d->titleLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->titleLabel->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());
    d->bodyLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->bodyLabel->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                     Ui::DesignSystem::layout().px12(),
                                     Ui::DesignSystem::layout().px12());
    d->readMoreLink->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onPrimary(), Ui::DesignSystem::disabledTextOpacity()));
    d->readMoreLink->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                        Ui::DesignSystem::layout().px12(), 0);
    d->installButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->installButton->setTextColor(Ui::DesignSystem::color().secondary());

    d->buttonsLayout->setContentsMargins(Ui::DesignSystem::layout().px8(),
                                         Ui::DesignSystem::layout().px4(),
                                         Ui::DesignSystem::layout().px8(), 0);
}

} // namespace Ui
