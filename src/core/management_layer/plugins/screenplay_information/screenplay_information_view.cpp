#include "screenplay_information_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui
{

class ScreenplayInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* screenplayInfo = nullptr;
    QGridLayout* screenplayInfoLayout = nullptr;
    TextField* screenplayName = nullptr;
    TextField* screenplayHeader = nullptr;
    TextField* screenplayFooter = nullptr;
    TextField* scenesNumbersPrefix = nullptr;
    TextField* scenesNumberingStartAt = nullptr;
};

ScreenplayInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      screenplayInfo(new Card(_parent)),
      screenplayInfoLayout(new QGridLayout),
      screenplayName(new TextField(screenplayInfo)),
      screenplayHeader(new TextField(screenplayInfo)),
      screenplayFooter(new TextField(screenplayInfo)),
      scenesNumbersPrefix(new TextField(screenplayInfo)),
      scenesNumberingStartAt(new TextField(screenplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setVerticalScrollBar(new ScrollBar);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    screenplayInfoLayout->setContentsMargins({});
    screenplayInfoLayout->setSpacing(0);
    screenplayInfoLayout->setRowMinimumHeight(0, 1); // добавляем пустую строку сверху
    screenplayInfoLayout->addWidget(screenplayName, 1, 0);
    screenplayInfoLayout->addWidget(screenplayHeader, 2, 0);
    screenplayInfoLayout->addWidget(screenplayFooter, 3, 0);
    screenplayInfoLayout->addWidget(scenesNumbersPrefix, 4, 0);
    screenplayInfoLayout->addWidget(scenesNumberingStartAt, 5, 0);
    screenplayInfoLayout->setRowMinimumHeight(6, 1); // добавляем пустую строку внизу
    screenplayInfoLayout->setColumnStretch(0, 1);
    screenplayInfo->setLayoutReimpl(screenplayInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(screenplayInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ScreenplayInformationView::ScreenplayInformationView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->screenplayName, &TextField::textChanged, this, [this] {
        emit nameChanged(d->screenplayName->text());
    });
    connect(d->screenplayHeader, &TextField::textChanged, this, [this] {
        emit headerChanged(d->screenplayHeader->text());
    });
    connect(d->screenplayFooter, &TextField::textChanged, this, [this] {
        emit footerChanged(d->screenplayFooter->text());
    });
    connect(d->scenesNumbersPrefix, &TextField::textChanged, this, [this] {
        emit scenesNumbersPrefixChanged(d->scenesNumbersPrefix->text());
    });
    connect(d->scenesNumberingStartAt, &TextField::textChanged, this, [this] {
        bool isNumberValid = false;
        const auto startNumber = d->scenesNumberingStartAt->text().toInt(&isNumberValid);
        if (isNumberValid) {
            emit scenesNumberingStartAtChanged(startNumber);
        } else {
            d->scenesNumberingStartAt->undo();
        }
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayInformationView::~ScreenplayInformationView() = default;

void ScreenplayInformationView::setName(const QString& _name)
{
    d->screenplayName->setText(_name);
}

void ScreenplayInformationView::setHeader(const QString& _header)
{
    d->screenplayHeader->setText(_header);
}

void ScreenplayInformationView::setFooter(const QString& _footer)
{
    d->screenplayFooter->setText(_footer);
}

void ScreenplayInformationView::setScenesNumbersPrefix(const QString& _prefix)
{
    d->scenesNumbersPrefix->setText(_prefix);
}

void ScreenplayInformationView::setScenesNumbersingStartAt(int _startNumber)
{
    d->scenesNumberingStartAt->setText(QString::number(_startNumber));
}

void ScreenplayInformationView::updateTranslations()
{
    d->screenplayName->setLabel(tr("Screenplay name"));
    d->screenplayHeader->setLabel(tr("Header"));
    d->screenplayFooter->setLabel(tr("Footer"));
    d->scenesNumbersPrefix->setLabel(tr("Scenes numbers' prefix"));
    d->scenesNumberingStartAt->setLabel(tr("Scenes numbering start at"));
}

void ScreenplayInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().topContentMargin(),
                          Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24())
                .toMargins());

    d->screenplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->screenplayName,
                            d->screenplayHeader,
                            d->screenplayFooter,
                            d->scenesNumbersPrefix,
                            d->scenesNumberingStartAt}) {
        textField->setBackgroundColor(Ui::DesignSystem::color().background());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->screenplayInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->screenplayInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->screenplayInfoLayout->setRowMinimumHeight(6, static_cast<int>(Ui::DesignSystem::layout().px16()));
}

} // namespace Ui
