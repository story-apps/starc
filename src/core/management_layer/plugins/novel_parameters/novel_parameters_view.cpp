#include "novel_parameters_view.h"

#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/templates_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QStandardItemModel>


namespace Ui {

class NovelParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* novelInfo = nullptr;
    QVBoxLayout* infoLayout = nullptr;
    TextField* header = nullptr;
    CheckBox* printHeaderOnTitlePage = nullptr;
    TextField* footer = nullptr;
    CheckBox* printFooterOnTitlePage = nullptr;
    CheckBox* overrideCommonSettings = nullptr;
    ComboBox* novelTemplate = nullptr;
};

NovelParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , novelInfo(new Card(_parent))
    , infoLayout(new QVBoxLayout)
    , header(new TextField(novelInfo))
    , printHeaderOnTitlePage(new CheckBox(novelInfo))
    , footer(new TextField(novelInfo))
    , printFooterOnTitlePage(new CheckBox(novelInfo))
    , overrideCommonSettings(new CheckBox(novelInfo))
    , novelTemplate(new ComboBox(_parent))
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
    UiHelper::initOptionsFor({
        header,
        footer,
    });

    novelTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    novelTemplate->setModel(BusinessLayer::TemplatesFacade::novelTemplates());
    novelTemplate->hide();

    infoLayout->setDirection(QBoxLayout::TopToBottom);
    infoLayout->setContentsMargins({});
    infoLayout->setSpacing(0);
    infoLayout->addWidget(header);
    infoLayout->addWidget(printHeaderOnTitlePage);
    infoLayout->addWidget(footer);
    infoLayout->addWidget(printFooterOnTitlePage);
    infoLayout->addWidget(overrideCommonSettings);
    infoLayout->addWidget(novelTemplate);
    novelInfo->setContentLayout(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(novelInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


NovelParametersView::NovelParametersView(QWidget* _parent)
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
            &NovelParametersView::printHeaderOnTitlePageChanged);
    connect(d->footer, &TextField::textChanged, this,
            [this] { emit footerChanged(d->footer->text()); });
    connect(d->printFooterOnTitlePage, &CheckBox::checkedChanged, this,
            &NovelParametersView::printFooterOnTitlePageChanged);
    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            &NovelParametersView::overrideCommonSettingsChanged);
    connect(d->novelTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto templateId
                    = _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
                emit novelTemplateChanged(templateId);
            });

    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            [this](bool _checked) { d->novelTemplate->setVisible(_checked); });
}

NovelParametersView::~NovelParametersView() = default;

QWidget* NovelParametersView::asQWidget()
{
    return this;
}

void NovelParametersView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->header->setReadOnly(readOnly);
    d->footer->setReadOnly(readOnly);
    d->novelTemplate->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->printHeaderOnTitlePage->setEnabled(enabled);
    d->printFooterOnTitlePage->setEnabled(enabled);
    d->overrideCommonSettings->setEnabled(enabled);
}

void NovelParametersView::setHeader(const QString& _header)
{
    if (d->header->text() == _header) {
        return;
    }

    d->header->setText(_header);
}

void NovelParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printHeaderOnTitlePage->setChecked(_print);
}

void NovelParametersView::setFooter(const QString& _footer)
{
    if (d->footer->text() == _footer) {
        return;
    }

    d->footer->setText(_footer);
}

void NovelParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printFooterOnTitlePage->setChecked(_print);
}

void NovelParametersView::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings->isChecked() == _override) {
        return;
    }

    d->overrideCommonSettings->setChecked(_override);
}

void NovelParametersView::setNovelTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::novelTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::novelTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        if (d->novelTemplate->currentIndex() != item->index()) {
            d->novelTemplate->setCurrentIndex(item->index());
        }
        break;
    }
}

void NovelParametersView::updateTranslations()
{
    d->header->setLabel(tr("Header"));
    d->printHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->footer->setLabel(tr("Footer"));
    d->printFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->overrideCommonSettings->setText(tr("Override common settings for this novel"));
    d->novelTemplate->setLabel(tr("Template"));
}

void NovelParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->novelInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->header,
             d->footer,
             d->novelTemplate,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->novelTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto checkBox : {
             d->printHeaderOnTitlePage,
             d->printFooterOnTitlePage,
             d->overrideCommonSettings,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->infoLayout->setSpacing(static_cast<int>(Ui::DesignSystem::compactLayout().px16()));
    d->infoLayout->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                      static_cast<int>(Ui::DesignSystem::layout().px12()));
}

} // namespace Ui
