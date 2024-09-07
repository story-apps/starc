#include "compare_subscriptions_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/shadow/shadow.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QScrollArea>


namespace Ui {

class CompareSubscriptionsDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* freeTitleIcon = nullptr;
    ButtonLabel* freeTitle = nullptr;
    Subtitle2Label* freeSubtitle = nullptr;
    //
    IconsMidLabel* proTitleIcon = nullptr;
    ButtonLabel* proTitle = nullptr;
    Subtitle2Label* proSubtitle = nullptr;
    //
    IconsMidLabel* cloudTitleIcon = nullptr;
    ButtonLabel* cloudTitle = nullptr;
    Subtitle2Label* cloudSubtitle = nullptr;
    //
    QScrollArea* content = nullptr;
    QGridLayout* contentLayout = nullptr;
    QVector<QVector<QWidget*>> contentWidgets;
    //
    Button* buyPro = nullptr;
    Button* giftPro = nullptr;
    Button* buyCloud = nullptr;
    Button* giftCloud = nullptr;
    Button* cancelButton = nullptr;
};

CompareSubscriptionsDialog::Implementation::Implementation(QWidget* _parent)
    : freeTitleIcon(new IconsMidLabel(_parent))
    , freeTitle(new ButtonLabel(_parent))
    , freeSubtitle(new Subtitle2Label(_parent))
    , proTitleIcon(new IconsMidLabel(_parent))
    , proTitle(new ButtonLabel(_parent))
    , proSubtitle(new Subtitle2Label(_parent))
    , cloudTitleIcon(new IconsMidLabel(_parent))
    , cloudTitle(new ButtonLabel(_parent))
    , cloudSubtitle(new Subtitle2Label(_parent))
    , content(UiHelper::createScrollArea(_parent, true))
    , buyPro(new Button(_parent))
    , giftPro(new Button(_parent))
    , buyCloud(new Button(_parent))
    , giftCloud(new Button(_parent))
    , cancelButton(new Button(_parent))
{
    freeTitleIcon->setIcon(u8"\U000F0381");
    freeSubtitle->setAlignment(Qt::AlignCenter);
    proTitleIcon->setIcon(u8"\U000F18BC");
    proSubtitle->setAlignment(Qt::AlignCenter);
    cloudTitleIcon->setIcon(u8"\U000F015F");
    cloudSubtitle->setAlignment(Qt::AlignCenter);
    contentLayout = qobject_cast<QGridLayout*>(content->widget()->layout());

    new Shadow(Qt::TopEdge, content);
    new Shadow(Qt::BottomEdge, content);


    //
    // Настраиваем список фич для сравнения версий
    // NOTE: т.к. диалог показывается когда язык интерфейса уже выбран, формируем текстовое
    //       наполнение прямо в конструкторе
    //
    struct Row {
        QString text = {};
        bool free = false;
        bool pro = false;
        bool cloud = false;

        bool isTitle() const
        {
            return !free && !pro && !cloud;
        }
    };
    const QVector<Row> table = {
        { tr("Unlimited number of projects"), true, true, true },
        { tr("Unlimited number of documents per project"), true, true, true },
        { tr("Cover generator"), true, true, true },
        { tr("Folders"), true, true, true },
        { tr("Text documents"), true, true, true },
        { tr("Session statistics"), true, true, true },
        { tr("Mind maps"), false, true, true },
        { tr("Images gallery"), false, true, true },
        { tr("Seamless synchronization across all your devices"), false, false, true },
        { tr("Realtime collaboration"), false, false, true },
        { tr("5GB cloud storage"), false, false, true },
        { tr("Character") },
        { tr("Basic info"), true, true, true },
        { tr("Export"), true, true, true },
        { tr("Characters relations"), false, true, true },
        { tr("Extended info"), false, true, true },
        { tr("Dialogues from all stories"), false, true, true },
        { tr("Location") },
        { tr("Basic info"), true, true, true },
        { tr("Export"), true, true, true },
        { tr("Locations map"), false, true, true },
        { tr("Extended info"), false, true, true },
        { tr("Scenes from all stories"), false, true, true },
        { tr("World") },
        { tr("Basic info"), true, true, true },
        { tr("Worlds map"), false, true, true },
        { tr("Extended info"), false, true, true },
        { tr("Screenplay") },
        { tr("Logline & synopsis"), true, true, true },
        { tr("Title page"), true, true, true },
        { tr("Treatment text"), true, true, true },
        { tr("Script text"), true, true, true },
        { tr("Basic statistics"), true, true, true },
        { tr("Import/export"), true, true, true },
        { tr("Scenes numbers locking"), true, true, true },
        { tr("Corkboard"), false, true, true },
        { tr("Timeline"), false, true, true },
        { tr("Extended statistics"), false, true, true },
        { tr("Script breakdown"), false, false, true },
        { tr("Comics") },
        { tr("Logline & synopsis"), true, true, true },
        { tr("Title page"), true, true, true },
        { tr("Script text"), true, true, true },
        { tr("Basic statistics"), true, true, true },
        { tr("Import/export"), true, true, true },
        { tr("Audio drama") },
        { tr("Logline & synopsis"), true, true, true },
        { tr("Title page"), true, true, true },
        { tr("Script text"), true, true, true },
        { tr("Import/export"), true, true, true },
        { tr("Extended statistics"), false, true, true },
        { tr("Stageplay") },
        { tr("Logline & synopsis"), true, true, true },
        { tr("Title page"), true, true, true },
        { tr("Script text"), true, true, true },
        { tr("Import/export"), true, true, true },
        { tr("Novel") },
        { tr("Logline & synopsis"), true, true, true },
        { tr("Title page"), true, true, true },
        { tr("Outline text"), true, true, true },
        { tr("Novel text"), true, true, true },
        { tr("Basic statistics"), true, true, true },
        { tr("Import/export"), true, true, true },
        { tr("Corkboard"), false, true, true },
        { tr("Timeline"), false, true, true },
    };
    int row = 0;
    const auto unavailableIconColor = ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                               DesignSystem::inactiveItemOpacity());
    for (const auto& rowData : table) {
        if (rowData.isTitle()) {
            auto label = new H6Label;
            label->setBackgroundColor(DesignSystem::color().background());
            label->setTextColor(DesignSystem::color().onBackground());
            label->setContentsMargins(DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                      DesignSystem::layout().px24(), DesignSystem::layout().px24());
            label->setText(rowData.text);
            contentLayout->addWidget(label, row, 0, 1, 4);
        } else {
            auto label = new Body2Label;
            label->setBackgroundColor(DesignSystem::color().background());
            label->setTextColor(DesignSystem::color().onBackground());
            label->setContentsMargins(DesignSystem::layout().px24(), DesignSystem::layout().px16(),
                                      DesignSystem::layout().px24(), DesignSystem::layout().px16());
            label->setText(rowData.text);
            contentLayout->addWidget(label, row, 0);
            //
            auto free = new IconsMidLabel;
            free->setBackgroundColor(DesignSystem::color().background());
            free->setTextColor(rowData.free ? DesignSystem::color().accent()
                                            : unavailableIconColor);
            free->setIcon(rowData.free ? u8"\U000F012C" : u8"\U000F0374");
            contentLayout->addWidget(free, row, 1, Qt::AlignCenter);
            //
            auto pro = new IconsMidLabel;
            pro->setBackgroundColor(DesignSystem::color().background());
            pro->setTextColor(rowData.pro ? DesignSystem::color().accent() : unavailableIconColor);
            pro->setIcon(rowData.pro ? u8"\U000F012C" : u8"\U000F0374");
            contentLayout->addWidget(pro, row, 2, Qt::AlignCenter);
            //
            auto cloud = new IconsMidLabel;
            cloud->setBackgroundColor(DesignSystem::color().background());
            cloud->setTextColor(rowData.cloud ? DesignSystem::color().accent()
                                              : unavailableIconColor);
            cloud->setIcon(rowData.cloud ? u8"\U000F012C" : u8"\U000F0374");
            contentLayout->addWidget(cloud, row, 3, Qt::AlignCenter);

            contentWidgets.append({ label, free, pro, cloud });
        }

        ++row;
    }
    contentLayout->setColumnStretch(0, 1);
    contentLayout->setColumnStretch(1, 1);
    contentLayout->setColumnStretch(2, 1);
    contentLayout->setColumnStretch(3, 1);
}


// ****


CompareSubscriptionsDialog::CompareSubscriptionsDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setRejectButton(d->cancelButton);

    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addStretch(1);
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addStretch();
        titleLayout->addWidget(d->freeTitleIcon);
        titleLayout->addWidget(d->freeTitle, 0, Qt::AlignVCenter);
        titleLayout->addStretch();
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->freeSubtitle, 0, Qt::AlignCenter);
        topLayout->addLayout(layout, 1);
    }
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addStretch();
        titleLayout->addWidget(d->proTitleIcon);
        titleLayout->addWidget(d->proTitle, 0, Qt::AlignVCenter);
        titleLayout->addStretch();
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->proSubtitle, 0, Qt::AlignCenter);
        topLayout->addLayout(layout, 1);
    }
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addStretch();
        titleLayout->addWidget(d->cloudTitleIcon);
        titleLayout->addWidget(d->cloudTitle, 0, Qt::AlignVCenter);
        titleLayout->addStretch();
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->cloudSubtitle, 0, Qt::AlignCenter);
        topLayout->addLayout(layout, 1);
    }

    auto buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(d->cancelButton, 1, Qt::AlignCenter);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(d->buyPro, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->giftPro, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->buyCloud, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->giftCloud, 1, Qt::AlignCenter);

    int row = 0;
    contentsLayout()->addLayout(topLayout, row++, 0);
    contentsLayout()->addWidget(d->content, row++, 0);
    contentsLayout()->addLayout(buttonsLayout, row++, 0);

    connect(d->cancelButton, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->buyPro, &Button::clicked, this, &CompareSubscriptionsDialog::purchaseProPressed);
    connect(d->buyPro, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->giftPro, &Button::clicked, this, &CompareSubscriptionsDialog::giftProPressed);
    connect(d->giftPro, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->buyCloud, &Button::clicked, this, &CompareSubscriptionsDialog::purchaseCloudPressed);
    connect(d->buyCloud, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->giftCloud, &Button::clicked, this, &CompareSubscriptionsDialog::giftCloudPressed);
    connect(d->giftCloud, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
}

CompareSubscriptionsDialog::~CompareSubscriptionsDialog() = default;

void CompareSubscriptionsDialog::setLifetimeOptions(bool _hasPro, bool _hasCloud)
{
    d->buyPro->setVisible(!_hasPro);
    d->giftPro->setVisible(_hasPro);
    d->buyCloud->setVisible(!_hasCloud);
    d->giftCloud->setVisible(_hasCloud);
}

void CompareSubscriptionsDialog::setConnected(bool _connected)
{
    d->buyPro->setEnabled(_connected);
    d->giftPro->setEnabled(_connected);
    d->buyCloud->setEnabled(_connected);
    d->giftCloud->setEnabled(_connected);
}

QWidget* CompareSubscriptionsDialog::focusedWidgetAfterShow() const
{
    return d->content;
}

QWidget* CompareSubscriptionsDialog::lastFocusableWidget() const
{
    return d->buyCloud;
}

void CompareSubscriptionsDialog::updateTranslations()
{
    d->freeTitle->setText("FREE");
    d->freeSubtitle->setText(tr("For those who are at the beginning of creative journey"));
    d->proTitle->setText("PRO");
    d->proSubtitle->setText(tr("Advanced tools for professionals"));
    d->cloudTitle->setText("CLOUD");
    d->cloudSubtitle->setText(tr("For those who are always on the move"));
    d->buyPro->setText(tr("Renew"));
    d->giftPro->setText(tr("Buy as a gift"));
    d->buyCloud->setText(tr("Renew"));
    d->giftCloud->setText(tr("Buy as a gift"));
}

void CompareSubscriptionsDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().background());
    setContentMinimumWidth(DesignSystem::dialog().maximumWidth());
    setContentMaximumWidth(parentWidget()->width() * 0.6);

    for (auto label : std::vector<Widget*>{
             d->freeTitleIcon,
             d->freeTitle,
             d->proTitleIcon,
             d->proTitle,
             d->cloudTitleIcon,
             d->cloudTitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
    }
    for (auto label : {
             d->freeTitleIcon,
             d->proTitleIcon,
             d->cloudTitleIcon,
         }) {
        label->setContentsMargins(DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                  DesignSystem::layout().px12(), DesignSystem::layout().px12());
    }
    for (auto label : {
             d->freeTitle,
             d->proTitle,
             d->cloudTitle,
         }) {
        label->setContentsMargins(0, DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                  DesignSystem::layout().px12());
    }

    for (auto label : {
             d->freeSubtitle,
             d->proSubtitle,
             d->cloudSubtitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                     DesignSystem::inactiveTextOpacity()));
        label->setContentsMargins(DesignSystem::layout().px24(), 0, DesignSystem::layout().px24(),
                                  DesignSystem::layout().px24());
    }

    for (auto button : {
             d->buyPro,
             d->giftPro,
             d->buyCloud,
             d->giftCloud,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
        button->setContentsMargins(0, DesignSystem::layout().px12(), 0,
                                   DesignSystem::layout().px12());
    }
    d->cancelButton->setBackgroundColor(DesignSystem::color().background());
    d->cancelButton->setTextColor(DesignSystem::color().background());
}

} // namespace Ui
