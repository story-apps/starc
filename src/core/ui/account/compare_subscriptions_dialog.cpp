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
    IconsMidLabel* creatorTitleIcon = nullptr;
    ButtonLabel* creatorTitle = nullptr;
    Subtitle2Label* creatorSubtitle = nullptr;
    //
    QScrollArea* content = nullptr;
    QGridLayout* contentLayout = nullptr;
    QVector<QVector<QWidget*>> contentWidgets;
    //
    Button* buyPro = nullptr;
    Button* giftPro = nullptr;
    Button* buyCreator = nullptr;
    Button* giftCreator = nullptr;
    Button* cancelButton = nullptr;
};

CompareSubscriptionsDialog::Implementation::Implementation(QWidget* _parent)
    : freeTitleIcon(new IconsMidLabel(_parent))
    , freeTitle(new ButtonLabel(_parent))
    , freeSubtitle(new Subtitle2Label(_parent))
    , proTitleIcon(new IconsMidLabel(_parent))
    , proTitle(new ButtonLabel(_parent))
    , proSubtitle(new Subtitle2Label(_parent))
    , creatorTitleIcon(new IconsMidLabel(_parent))
    , creatorTitle(new ButtonLabel(_parent))
    , creatorSubtitle(new Subtitle2Label(_parent))
    , content(UiHelper::createScrollAreaWithGridLayout(_parent))
    , buyPro(new Button(_parent))
    , giftPro(new Button(_parent))
    , buyCreator(new Button(_parent))
    , giftCreator(new Button(_parent))
    , cancelButton(new Button(_parent))
{
    freeTitleIcon->setIcon(u8"\U000F0381");
    freeSubtitle->setAlignment(Qt::AlignCenter);
    proTitleIcon->setIcon(u8"\U000F18BC");
    proSubtitle->setAlignment(Qt::AlignCenter);
    creatorTitleIcon->setIcon(u8"\U000F0674");
    creatorSubtitle->setAlignment(Qt::AlignCenter);
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
        bool creator = false;

        bool isTitle() const
        {
            return !free && !pro && !creator;
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
        { tr("Series plan"), false, false, true },
        { tr("Series statistics"), false, false, true },
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
            auto creator = new IconsMidLabel;
            creator->setBackgroundColor(DesignSystem::color().background());
            creator->setTextColor(rowData.creator ? DesignSystem::color().accent()
                                                  : unavailableIconColor);
            creator->setIcon(rowData.creator ? u8"\U000F012C" : u8"\U000F0374");
            contentLayout->addWidget(creator, row, 3, Qt::AlignCenter);

            contentWidgets.append({ label, free, pro, creator });
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
        titleLayout->addWidget(d->creatorTitleIcon);
        titleLayout->addWidget(d->creatorTitle, 0, Qt::AlignVCenter);
        titleLayout->addStretch();
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->creatorSubtitle, 0, Qt::AlignCenter);
        topLayout->addLayout(layout, 1);
    }

    auto buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(d->cancelButton, 1, Qt::AlignVCenter | Qt::AlignLeft);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(d->buyPro, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->giftPro, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->buyCreator, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->giftCreator, 1, Qt::AlignCenter);

    int row = 0;
    contentsLayout()->addLayout(topLayout, row++, 0);
    contentsLayout()->addWidget(d->content, row++, 0);
    contentsLayout()->addLayout(buttonsLayout, row++, 0);

    connect(d->cancelButton, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->buyPro, &Button::clicked, this, &CompareSubscriptionsDialog::purchaseProPressed);
    connect(d->buyPro, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->giftPro, &Button::clicked, this, &CompareSubscriptionsDialog::giftProPressed);
    connect(d->giftPro, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->buyCreator, &Button::clicked, this,
            &CompareSubscriptionsDialog::purchaseCreatorPressed);
    connect(d->buyCreator, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
    connect(d->giftCreator, &Button::clicked, this,
            &CompareSubscriptionsDialog::giftCreatorPressed);
    connect(d->giftCreator, &Button::clicked, this, &CompareSubscriptionsDialog::hideDialog);
}

CompareSubscriptionsDialog::~CompareSubscriptionsDialog() = default;

void CompareSubscriptionsDialog::setLifetimeOptions(bool _hasPro, bool _hasCreator)
{
    d->buyPro->setVisible(!_hasPro);
    d->giftPro->setVisible(_hasPro);
    d->buyCreator->setVisible(!_hasCreator);
    d->giftCreator->setVisible(_hasCreator);
}

void CompareSubscriptionsDialog::setConnected(bool _connected)
{
    d->buyPro->setEnabled(_connected);
    d->giftPro->setEnabled(_connected);
    d->buyCreator->setEnabled(_connected);
    d->giftCreator->setEnabled(_connected);
}

QWidget* CompareSubscriptionsDialog::focusedWidgetAfterShow() const
{
    return d->content;
}

QWidget* CompareSubscriptionsDialog::lastFocusableWidget() const
{
    return d->buyCreator;
}

void CompareSubscriptionsDialog::updateTranslations()
{
    d->freeTitle->setText("FREE");
    d->freeSubtitle->setText(tr("For those who are at the beginning of creative journey"));
    d->proTitle->setText("PRO");
    d->proSubtitle->setText(tr("Advanced tools for professionals"));
    d->creatorTitle->setText("CREATOR");
    d->creatorSubtitle->setText(tr("The next level tools for unlimited creativity"));
    d->buyPro->setText(tr("Activate"));
    d->giftPro->setText(tr("Buy as a gift"));
    d->buyCreator->setText(tr("Activate"));
    d->giftCreator->setText(tr("Buy as a gift"));
    d->cancelButton->setText(tr("Cancel"));
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
             d->creatorTitleIcon,
             d->creatorTitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
    }
    for (auto label : {
             d->freeTitleIcon,
             d->proTitleIcon,
             d->creatorTitleIcon,
         }) {
        label->setContentsMargins(DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                  DesignSystem::layout().px12(), DesignSystem::layout().px12());
    }
    for (auto label : {
             d->freeTitle,
             d->proTitle,
             d->creatorTitle,
         }) {
        label->setContentsMargins(0, DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                  DesignSystem::layout().px12());
    }

    for (auto label : {
             d->freeSubtitle,
             d->proSubtitle,
             d->creatorSubtitle,
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
             d->buyCreator,
             d->giftCreator,
         }) {
        UiHelper::initColorsFor(button, UiHelper::DialogAccept);
        button->setContentsMargins(0, DesignSystem::layout().px12(), 0,
                                   DesignSystem::layout().px12());
    }
    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    d->cancelButton->setContentsMargins(
        DesignSystem::layout().px12(), DesignSystem::layout().px12(), DesignSystem::layout().px12(),
        DesignSystem::layout().px12());
}

} // namespace Ui
