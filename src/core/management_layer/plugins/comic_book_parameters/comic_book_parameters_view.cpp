#include "comic_book_parameters_view.h"

#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QStandardItemModel>


namespace Ui {

class ComicBookParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* comicBookInfo = nullptr;
    QGridLayout* infoLayout = nullptr;
    TextField* header = nullptr;
    CheckBox* comicBookPrintHeaderOnTitlePage = nullptr;
    TextField* footer = nullptr;
    CheckBox* comicBookPrintFooterOnTitlePage = nullptr;
    CheckBox* overrideCommonSettings = nullptr;
    ComboBox* comicBookTemplate = nullptr;
};

ComicBookParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , comicBookInfo(new Card(_parent))
    , infoLayout(new QGridLayout)
    , header(new TextField(comicBookInfo))
    , comicBookPrintHeaderOnTitlePage(new CheckBox(comicBookInfo))
    , footer(new TextField(comicBookInfo))
    , comicBookPrintFooterOnTitlePage(new CheckBox(comicBookInfo))
    , overrideCommonSettings(new CheckBox(comicBookInfo))
    , comicBookTemplate(new ComboBox(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    comicBookInfo->setResizingActive(false);

    header->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    footer->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    comicBookTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    comicBookTemplate->setModel(BusinessLayer::TemplatesFacade::comicBookTemplates());
    comicBookTemplate->hide();

    infoLayout->setContentsMargins({});
    infoLayout->setSpacing(0);
    int row = 0;
    infoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    infoLayout->addWidget(header, row++, 0);
    infoLayout->addWidget(comicBookPrintHeaderOnTitlePage, row++, 0);
    infoLayout->addWidget(footer, row++, 0);
    infoLayout->addWidget(comicBookPrintFooterOnTitlePage, row++, 0);
    infoLayout->addWidget(overrideCommonSettings, row++, 0, Qt::AlignTop);
    infoLayout->addWidget(comicBookTemplate, row++, 0);
    infoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    infoLayout->setColumnStretch(0, 1);
    comicBookInfo->setLayoutReimpl(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(comicBookInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ComicBookParametersView::ComicBookParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->header, &TextField::textChanged, this,
            [this] { emit headerChanged(d->header->text()); });
    connect(d->comicBookPrintHeaderOnTitlePage, &CheckBox::checkedChanged, this,
            &ComicBookParametersView::printHeaderOnTitlePageChanged);
    connect(d->footer, &TextField::textChanged, this,
            [this] { emit footerChanged(d->footer->text()); });
    connect(d->comicBookPrintFooterOnTitlePage, &CheckBox::checkedChanged, this,
            &ComicBookParametersView::printFooterOnTitlePageChanged);
    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            &ComicBookParametersView::overrideCommonSettingsChanged);
    connect(d->comicBookTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto templateId
                    = _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
                emit comicBookTemplateChanged(templateId);
            });
    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            [this](bool _checked) { d->comicBookTemplate->setVisible(_checked); });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ComicBookParametersView::~ComicBookParametersView() = default;

QWidget* ComicBookParametersView::asQWidget()
{
    return this;
}

void ComicBookParametersView::setHeader(const QString& _header)
{
    if (d->header->text() == _header) {
        return;
    }

    d->header->setText(_header);
}

void ComicBookParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->comicBookPrintHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->comicBookPrintHeaderOnTitlePage->setChecked(_print);
}

void ComicBookParametersView::setFooter(const QString& _footer)
{
    if (d->footer->text() == _footer) {
        return;
    }

    d->footer->setText(_footer);
}

void ComicBookParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->comicBookPrintFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->comicBookPrintFooterOnTitlePage->setChecked(_print);
}

void ComicBookParametersView::setOverrideCommonSettings(bool _override)
{
    d->overrideCommonSettings->setChecked(_override);
}

void ComicBookParametersView::setComicBookTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::comicBookTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::comicBookTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->comicBookTemplate->setCurrentIndex(item->index());
        break;
    }
}

void ComicBookParametersView::updateTranslations()
{
    d->header->setLabel(tr("Header"));
    d->comicBookPrintHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->footer->setLabel(tr("Footer"));
    d->comicBookPrintFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->overrideCommonSettings->setText(tr("Override common settings for this comic book"));
    d->comicBookTemplate->setLabel(tr("Template"));
}

void ComicBookParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->comicBookInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->header,
             d->footer,
             d->comicBookTemplate,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->comicBookTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto checkBox : {
             d->comicBookPrintHeaderOnTitlePage,
             d->comicBookPrintFooterOnTitlePage,
             d->overrideCommonSettings,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->infoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->infoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->infoLayout->setRowMinimumHeight(7, static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
