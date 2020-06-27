#include "screenplay_information_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
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
    TextField* screenplayLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* outlineVisiblity = nullptr;
    CheckBox* screenplayTextVisiblity = nullptr;
    CheckBox* screenplayStatisticsVisiblity = nullptr;
};

ScreenplayInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      screenplayInfo(new Card(_parent)),
      screenplayInfoLayout(new QGridLayout),
      screenplayName(new TextField(screenplayInfo)),
      screenplayLogline(new TextField(screenplayInfo)),
      titlePageVisiblity(new CheckBox(screenplayInfo)),
      synopsisVisiblity(new CheckBox(screenplayInfo)),
      outlineVisiblity(new CheckBox(screenplayInfo)),
      screenplayTextVisiblity(new CheckBox(screenplayInfo)),
      screenplayStatisticsVisiblity(new CheckBox(screenplayInfo))
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
    screenplayInfoLayout->addWidget(screenplayLogline, 2, 0);
    screenplayInfoLayout->addWidget(titlePageVisiblity, 3, 0);
    screenplayInfoLayout->addWidget(synopsisVisiblity, 4, 0);
    screenplayInfoLayout->addWidget(outlineVisiblity, 5, 0);
    screenplayInfoLayout->addWidget(screenplayTextVisiblity, 6, 0);
    screenplayInfoLayout->addWidget(screenplayStatisticsVisiblity, 7, 0);
    screenplayInfoLayout->setRowMinimumHeight(8, 1); // добавляем пустую строку внизу
    screenplayInfoLayout->setColumnStretch(0, 1);
    screenplayInfo->setLayoutReimpl(screenplayInfoLayout);

    screenplayLogline->setEnterMakesNewLine(true);

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
    connect(d->screenplayLogline, &TextField::textChanged, this, [this] {
        emit loglineChanged(d->screenplayLogline->text());
    });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged,
            this, &ScreenplayInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged,
            this, &ScreenplayInformationView::synopsisVisibleChanged);
    connect(d->outlineVisiblity, &CheckBox::checkedChanged,
            this, &ScreenplayInformationView::outlineVisibleChanged);
    connect(d->screenplayTextVisiblity, &CheckBox::checkedChanged,
            this, &ScreenplayInformationView::screenplayTextVisibleChanged);
    connect(d->screenplayStatisticsVisiblity, &CheckBox::checkedChanged,
            this, &ScreenplayInformationView::screenplayStatisticsVisibleChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayInformationView::~ScreenplayInformationView() = default;

void ScreenplayInformationView::setName(const QString& _name)
{
    if (d->screenplayName->text() == _name) {
        return;
    }

    d->screenplayName->setText(_name);
}

void ScreenplayInformationView::setLogline(const QString& _logline)
{
    if (d->screenplayLogline->text() == _logline) {
        return;
    }

    d->screenplayLogline->setText(_logline);
}

void ScreenplayInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void ScreenplayInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void ScreenplayInformationView::setOutlineVisible(bool _visible)
{
    d->outlineVisiblity->setChecked(_visible);
}

void ScreenplayInformationView::setScreenplayTextVisible(bool _visible)
{
    d->screenplayTextVisiblity->setChecked(_visible);
}

void ScreenplayInformationView::setScreenplayStatisticsVisible(bool _visible)
{
    d->screenplayStatisticsVisiblity->setChecked(_visible);
}

void ScreenplayInformationView::updateTranslations()
{
    d->screenplayName->setLabel(tr("Screenplay name"));
    d->screenplayLogline->setLabel(tr("Logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->outlineVisiblity->setText(tr("Outline"));
    d->screenplayTextVisiblity->setText(tr("Text"));
    d->screenplayStatisticsVisiblity->setText(tr("Statistics"));
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
                            d->screenplayLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->titlePageVisiblity,
                           d->synopsisVisiblity,
                           d->outlineVisiblity,
                           d->screenplayTextVisiblity,
                           d->screenplayStatisticsVisiblity }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->screenplayInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->screenplayInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->screenplayInfoLayout->setRowMinimumHeight(8, static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
