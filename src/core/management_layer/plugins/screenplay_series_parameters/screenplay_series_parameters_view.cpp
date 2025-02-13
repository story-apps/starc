#include "screenplay_series_parameters_view.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QStandardItemModel>


namespace Ui {

class ScreenplaySeriesParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* seriesInfo = nullptr;
    QVBoxLayout* infoLayout = nullptr;
    TextField* header = nullptr;
    CheckBox* printHeaderOnTitlePage = nullptr;
    TextField* footer = nullptr;
    CheckBox* printFooterOnTitlePage = nullptr;

    CheckBox* overrideCommonSettings = nullptr;
    ComboBox* screenplayTemplate = nullptr;
    CheckBox* showSceneNumbers = nullptr;
    CheckBox* showSceneNumbersOnLeft = nullptr;
    CheckBox* showSceneNumbersOnRight = nullptr;
    CheckBox* showDialoguesNumbers = nullptr;
};

ScreenplaySeriesParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , seriesInfo(new Card(_parent))
    , infoLayout(new QVBoxLayout)
    , header(new TextField(seriesInfo))
    , printHeaderOnTitlePage(new CheckBox(seriesInfo))
    , footer(new TextField(seriesInfo))
    , printFooterOnTitlePage(new CheckBox(seriesInfo))
    , overrideCommonSettings(new CheckBox(seriesInfo))
    , screenplayTemplate(new ComboBox(seriesInfo))
    , showSceneNumbers(new CheckBox(seriesInfo))
    , showSceneNumbersOnLeft(new CheckBox(seriesInfo))
    , showSceneNumbersOnRight(new CheckBox(seriesInfo))
    , showDialoguesNumbers(new CheckBox(seriesInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    header->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    footer->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    screenplayTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayTemplate->setModel(BusinessLayer::TemplatesFacade::screenplayTemplates());
    screenplayTemplate->hide();

    showSceneNumbers->hide();
    showSceneNumbersOnLeft->setEnabled(false);
    showSceneNumbersOnLeft->hide();
    showSceneNumbersOnRight->setEnabled(false);
    showSceneNumbersOnRight->hide();
    showDialoguesNumbers->hide();

    infoLayout->setDirection(QBoxLayout::TopToBottom);
    infoLayout->setContentsMargins({});
    infoLayout->setSpacing(0);
    infoLayout->addWidget(header);
    infoLayout->addWidget(printHeaderOnTitlePage);
    infoLayout->addWidget(footer);
    infoLayout->addWidget(printFooterOnTitlePage);
    infoLayout->addWidget(overrideCommonSettings, 1, Qt::AlignTop);
    infoLayout->addWidget(screenplayTemplate);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(showSceneNumbers);
        layout->addWidget(showSceneNumbersOnLeft);
        layout->addWidget(showSceneNumbersOnRight);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(showDialoguesNumbers);
    seriesInfo->setContentLayout(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(seriesInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ScreenplaySeriesParametersView::ScreenplaySeriesParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->header, &TextField::textChanged, this,
            [this] { emit headerChanged(d->header->text()); });
    connect(d->printHeaderOnTitlePage, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::printHeaderOnTitlePageChanged);
    connect(d->footer, &TextField::textChanged, this,
            [this] { emit footerChanged(d->footer->text()); });
    connect(d->printFooterOnTitlePage, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::printFooterOnTitlePageChanged);
    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::overrideCommonSettingsChanged);
    connect(d->screenplayTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto templateId
                    = _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
                emit screenplayTemplateChanged(templateId);
            });
    connect(d->showSceneNumbers, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::showSceneNumbersChanged);
    connect(d->showSceneNumbersOnLeft, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::showSceneNumbersOnLeftChanged);
    connect(d->showSceneNumbersOnRight, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::showSceneNumbersOnRightChanged);
    connect(d->showDialoguesNumbers, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesParametersView::showDialoguesNumbersChanged);

    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this, [this](bool _checked) {
        d->screenplayTemplate->setVisible(_checked);
        d->showSceneNumbers->setVisible(_checked);
        d->showSceneNumbersOnLeft->setVisible(_checked);
        d->showSceneNumbersOnRight->setVisible(_checked);
        d->showDialoguesNumbers->setVisible(_checked);
    });
    connect(d->showSceneNumbers, &CheckBox::checkedChanged, d->showSceneNumbersOnLeft,
            &CheckBox::setEnabled);
    connect(d->showSceneNumbers, &CheckBox::checkedChanged, d->showSceneNumbersOnRight,
            &CheckBox::setEnabled);
    auto correctShownSceneNumber = [this] {
        if (!d->showSceneNumbersOnLeft->isChecked() && !d->showSceneNumbersOnRight->isChecked()) {
            d->showSceneNumbersOnLeft->setChecked(true);
        }
    };
    connect(d->showSceneNumbersOnLeft, &CheckBox::checkedChanged, this, correctShownSceneNumber);
    connect(d->showSceneNumbersOnRight, &CheckBox::checkedChanged, this, correctShownSceneNumber);
}

ScreenplaySeriesParametersView::~ScreenplaySeriesParametersView() = default;

QWidget* ScreenplaySeriesParametersView::asQWidget()
{
    return this;
}

void ScreenplaySeriesParametersView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->header->setReadOnly(readOnly);
    d->footer->setReadOnly(readOnly);
    d->screenplayTemplate->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->printHeaderOnTitlePage->setEnabled(enabled);
    d->printFooterOnTitlePage->setEnabled(enabled);
    d->overrideCommonSettings->setEnabled(enabled);
    d->showSceneNumbers->setEnabled(enabled);
    d->showSceneNumbersOnLeft->setEnabled(enabled);
    d->showSceneNumbersOnRight->setEnabled(enabled);
    d->showDialoguesNumbers->setEnabled(enabled);
}

void ScreenplaySeriesParametersView::setHeader(const QString& _header)
{
    if (d->header->text() == _header) {
        return;
    }

    d->header->setText(_header);
}

void ScreenplaySeriesParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printHeaderOnTitlePage->setChecked(_print);
}

void ScreenplaySeriesParametersView::setFooter(const QString& _footer)
{
    if (d->footer->text() == _footer) {
        return;
    }

    d->footer->setText(_footer);
}

void ScreenplaySeriesParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printFooterOnTitlePage->setChecked(_print);
}

void ScreenplaySeriesParametersView::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings->isChecked() == _override) {
        return;
    }

    d->overrideCommonSettings->setChecked(_override);
}

void ScreenplaySeriesParametersView::setScreenplayTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::screenplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::screenplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        if (d->screenplayTemplate->currentIndex() != item->index()) {
            d->screenplayTemplate->setCurrentIndex(item->index());
        }
        break;
    }
}

void ScreenplaySeriesParametersView::setShowSceneNumbers(bool _show)
{
    if (d->showSceneNumbers->isChecked() == _show) {
        return;
    }

    d->showSceneNumbers->setChecked(_show);
}

void ScreenplaySeriesParametersView::setShowSceneNumbersOnLeft(bool _show)
{
    if (d->showSceneNumbersOnLeft->isChecked() == _show) {
        return;
    }

    d->showSceneNumbersOnLeft->setChecked(_show);
}

void ScreenplaySeriesParametersView::setShowSceneNumbersOnRight(bool _show)
{
    if (d->showSceneNumbersOnRight->isChecked() == _show) {
        return;
    }

    d->showSceneNumbersOnRight->setChecked(_show);
}

void ScreenplaySeriesParametersView::setShowDialoguesNumbers(bool _show)
{
    if (d->showDialoguesNumbers->isChecked() == _show) {
        return;
    }

    d->showDialoguesNumbers->setChecked(_show);
}

void ScreenplaySeriesParametersView::updateTranslations()
{
    d->header->setLabel(tr("Header"));
    d->printHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->footer->setLabel(tr("Footer"));
    d->printFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->overrideCommonSettings->setText(tr("Override common settings for this screenplay"));
    d->screenplayTemplate->setLabel(tr("Template"));
    d->showSceneNumbers->setText(tr("Print scenes numbers"));
    d->showSceneNumbersOnLeft->setText(tr("on the left"));
    d->showSceneNumbersOnRight->setText(tr("on the right"));
    d->showDialoguesNumbers->setText(tr("Print dialogues numbers"));
}

void ScreenplaySeriesParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->seriesInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->header,
             d->footer,
             d->screenplayTemplate,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->screenplayTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto checkBox : {
             d->printHeaderOnTitlePage,
             d->printFooterOnTitlePage,
             d->overrideCommonSettings,
             d->showSceneNumbers,
             d->showSceneNumbersOnLeft,
             d->showSceneNumbersOnRight,
             d->showDialoguesNumbers,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    auto titleMargins = Ui::DesignSystem::label().margins();
    titleMargins.setTop(0.0);
    titleMargins.setBottom(0.0);

    d->infoLayout->setSpacing(static_cast<int>(Ui::DesignSystem::compactLayout().px16()));
    d->infoLayout->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                      static_cast<int>(Ui::DesignSystem::layout().px12()));
}

} // namespace Ui
