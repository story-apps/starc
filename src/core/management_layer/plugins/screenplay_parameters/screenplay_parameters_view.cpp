#include "screenplay_parameters_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui
{

class ScreenplayParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* screenplayInfo = nullptr;
    QGridLayout* screenplayInfoLayout = nullptr;
    TextField* screenplayHeader = nullptr;
    CheckBox* screenplayPrintHeaderOnTitlePage = nullptr;
    TextField* screenplayFooter = nullptr;
    CheckBox* screenplayPrintFooterOnTitlePage = nullptr;
    TextField* scenesNumbersPrefix = nullptr;
    TextField* scenesNumberingStartAt = nullptr;
};

ScreenplayParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      screenplayInfo(new Card(_parent)),
      screenplayInfoLayout(new QGridLayout),
      screenplayHeader(new TextField(screenplayInfo)),
      screenplayPrintHeaderOnTitlePage(new CheckBox(screenplayInfo)),
      screenplayFooter(new TextField(screenplayInfo)),
      screenplayPrintFooterOnTitlePage(new CheckBox(screenplayInfo)),
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
    screenplayInfoLayout->addWidget(screenplayHeader, 1, 0);
    screenplayInfoLayout->addWidget(screenplayPrintHeaderOnTitlePage, 2, 0);
    screenplayInfoLayout->addWidget(screenplayFooter, 3, 0);
    screenplayInfoLayout->addWidget(screenplayPrintFooterOnTitlePage, 4, 0);
    screenplayInfoLayout->addWidget(scenesNumbersPrefix, 5, 0);
    screenplayInfoLayout->addWidget(scenesNumberingStartAt, 6, 0);
    screenplayInfoLayout->setRowMinimumHeight(7, 1); // добавляем пустую строку внизу
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


ScreenplayParametersView::ScreenplayParametersView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->screenplayHeader, &TextField::textChanged, this, [this] {
        emit headerChanged(d->screenplayHeader->text());
    });
    connect(d->screenplayPrintHeaderOnTitlePage, &CheckBox::checkedChanged,
            this, &ScreenplayParametersView::printHeaderOnTitlePageChanged);
    connect(d->screenplayFooter, &TextField::textChanged, this, [this] {
        emit footerChanged(d->screenplayFooter->text());
    });
    connect(d->screenplayPrintFooterOnTitlePage, &CheckBox::checkedChanged,
            this, &ScreenplayParametersView::printFooterOnTitlePageChanged);
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

ScreenplayParametersView::~ScreenplayParametersView() = default;


void ScreenplayParametersView::setHeader(const QString& _header)
{
    if (d->screenplayHeader->text() == _header) {
        return;
    }

    d->screenplayHeader->setText(_header);
}

void ScreenplayParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->screenplayPrintHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->screenplayPrintHeaderOnTitlePage->setChecked(_print);
}

void ScreenplayParametersView::setFooter(const QString& _footer)
{
    if (d->screenplayFooter->text() == _footer) {
        return;
    }

    d->screenplayFooter->setText(_footer);
}

void ScreenplayParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->screenplayPrintFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->screenplayPrintFooterOnTitlePage->setChecked(_print);
}

void ScreenplayParametersView::setScenesNumbersPrefix(const QString& _prefix)
{
    if (d->scenesNumbersPrefix->text() == _prefix) {
        return;
    }

    d->scenesNumbersPrefix->setText(_prefix);
}

void ScreenplayParametersView::setScenesNumbersingStartAt(int _startNumber)
{
    const auto startNumberText = QString::number(_startNumber);
    if (d->scenesNumberingStartAt->text() == startNumberText) {
        return;
    }

    d->scenesNumberingStartAt->setText(startNumberText);
}

void ScreenplayParametersView::updateTranslations()
{
    d->screenplayHeader->setLabel(tr("Header"));
    d->screenplayPrintHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->screenplayFooter->setLabel(tr("Footer"));
    d->screenplayPrintFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->scenesNumbersPrefix->setLabel(tr("Scenes numbers' prefix"));
    d->scenesNumberingStartAt->setLabel(tr("Scenes numbering start at"));
}

void ScreenplayParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
    for (auto textField : { d->screenplayHeader,
                            d->screenplayFooter,
                            d->scenesNumbersPrefix,
                            d->scenesNumberingStartAt }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->screenplayPrintHeaderOnTitlePage,
                            d->screenplayPrintFooterOnTitlePage }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->screenplayInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->screenplayInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->screenplayInfoLayout->setRowMinimumHeight(7, static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
