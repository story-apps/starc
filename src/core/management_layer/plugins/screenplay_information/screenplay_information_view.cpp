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
    TextField* screenplayLogline = nullptr;
};

ScreenplayInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      screenplayInfo(new Card(_parent)),
      screenplayInfoLayout(new QGridLayout),
      screenplayName(new TextField(screenplayInfo)),
      screenplayLogline(new TextField(screenplayInfo))
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
    screenplayInfoLayout->setRowMinimumHeight(3, 1); // добавляем пустую строку внизу
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

void ScreenplayInformationView::updateTranslations()
{
    d->screenplayName->setLabel(tr("Screenplay name"));
    d->screenplayLogline->setLabel(tr("Logline"));
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
    d->screenplayInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->screenplayInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->screenplayInfoLayout->setRowMinimumHeight(3, static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
