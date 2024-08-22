#include "compare_subscriptions_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
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
    Button* buyCloud = nullptr;
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
    , buyCloud(new Button(_parent))
{
    contentLayout = qobject_cast<QGridLayout*>(content->widget()->layout());
}


// ****


CompareSubscriptionsDialog::CompareSubscriptionsDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addStretch(1);
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(d->freeTitleIcon);
        titleLayout->addWidget(d->freeTitle);
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->freeSubtitle, 0, Qt::AlignLeft | Qt::AlignTop);
        topLayout->addLayout(layout, 1);
    }
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(d->proTitleIcon);
        titleLayout->addWidget(d->proTitle);
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->proSubtitle, 0, Qt::AlignLeft | Qt::AlignTop);
        topLayout->addLayout(layout, 1);
    }
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(d->cloudTitleIcon);
        titleLayout->addWidget(d->cloudTitle);
        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(d->cloudSubtitle, 0, Qt::AlignLeft | Qt::AlignTop);
        topLayout->addLayout(layout, 1);
    }

    auto buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch(1);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(d->buyPro, 1, Qt::AlignCenter);
    buttonsLayout->addWidget(d->buyCloud, 1, Qt::AlignCenter);

    int row = 0;
    contentsLayout()->addLayout(topLayout, row++, 0);
    contentsLayout()->addWidget(d->content, row++, 0);
    contentsLayout()->addLayout(buttonsLayout, row++, 0);
}

CompareSubscriptionsDialog::~CompareSubscriptionsDialog() = default;

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
    for (const auto& row : std::as_const(d->contentWidgets)) {
        for (auto widget : row) {
            d->contentLayout->removeWidget(widget);
            widget->deleteLater();
        }
    }
    d->contentWidgets.clear();

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
        { tr("Title") },
        { tr("Feature"), false, true, true },
    };
    int row = 0;
    for (const auto& rowData : table) {
        if (rowData.isTitle()) {

        } else {
            auto label = new Body1Label;
            label->setText(rowData.text);
            d->contentLayout->addWidget(label, row, 0);
            //
            auto free = new IconsMidLabel;
            free->setIcon(rowData.free ? u8"\U000F012C" : u8"\U000F0374");
            d->contentLayout->addWidget(free, row, 1);
            //
            auto pro = new IconsMidLabel;
            pro->setIcon(rowData.pro ? u8"\U000F012C" : u8"\U000F0374");
            d->contentLayout->addWidget(pro, row, 2);
            //
            auto cloud = new IconsMidLabel;
            cloud->setIcon(rowData.cloud ? u8"\U000F012C" : u8"\U000F0374");
            d->contentLayout->addWidget(cloud, row, 3);

            d->contentWidgets.append({ label, free, pro, cloud });
        }

        ++row;
    }
}

void CompareSubscriptionsDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
}

} // namespace Ui
