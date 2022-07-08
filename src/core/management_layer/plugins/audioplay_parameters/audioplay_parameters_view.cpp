#include "audioplay_parameters_view.h"

#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
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

class AudioplayParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* audioplayInfo = nullptr;
    QVBoxLayout* infoLayout = nullptr;
    TextField* header = nullptr;
    CheckBox* printHeaderOnTitlePage = nullptr;
    TextField* footer = nullptr;
    CheckBox* printFooterOnTitlePage = nullptr;
    CheckBox* overrideCommonSettings = nullptr;
    ComboBox* audioplayTemplate = nullptr;
    CheckBox* showBlockNumbers = nullptr;
    CheckBox* continueBlockNumbers = nullptr;
};

AudioplayParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , audioplayInfo(new Card(_parent))
    , infoLayout(new QVBoxLayout)
    , header(new TextField(audioplayInfo))
    , printHeaderOnTitlePage(new CheckBox(audioplayInfo))
    , footer(new TextField(audioplayInfo))
    , printFooterOnTitlePage(new CheckBox(audioplayInfo))
    , overrideCommonSettings(new CheckBox(audioplayInfo))
    , audioplayTemplate(new ComboBox(_parent))
    , showBlockNumbers(new CheckBox(_parent))
    , continueBlockNumbers(new CheckBox(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    audioplayInfo->setResizingActive(false);

    header->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    footer->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    audioplayTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    audioplayTemplate->setModel(BusinessLayer::TemplatesFacade::audioplayTemplates());
    audioplayTemplate->hide();

    showBlockNumbers->hide();
    continueBlockNumbers->setEnabled(false);
    continueBlockNumbers->hide();

    infoLayout->setDirection(QBoxLayout::TopToBottom);
    infoLayout->setContentsMargins({});
    infoLayout->setSpacing(0);
    infoLayout->addWidget(header);
    infoLayout->addWidget(printHeaderOnTitlePage);
    infoLayout->addWidget(footer);
    infoLayout->addWidget(printFooterOnTitlePage);
    infoLayout->addWidget(overrideCommonSettings, 1, Qt::AlignTop);
    infoLayout->addWidget(audioplayTemplate);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(showBlockNumbers);
        layout->addWidget(continueBlockNumbers);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    audioplayInfo->setLayoutReimpl(infoLayout);

    //
    // TODO: С лёту не завелось, т.к. при отображении скрытых виджетов, виджеты, которые были видны,
    // сжимаются лейаутом, что даёт некрасивый эффект дёргания (собственно это актуально и для
    // диалогов, просто там это не так сильно заметно как на больших карточках).
    //
    // Во время экспериментов не помогли ни фиксация размера виджета, ни установка минимального
    // размера строки лейаута, ни разные полтики лейута, надо смотреть код лейаута и искать лазейку,
    // как заставить его не сжимать некоторые из виджетов
    //
    audioplayInfo->setResizingActive(true);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(audioplayInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


AudioplayParametersView::AudioplayParametersView(QWidget* _parent)
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
    connect(d->printHeaderOnTitlePage, &CheckBox::checkedChanged, this,
            &AudioplayParametersView::printHeaderOnTitlePageChanged);
    connect(d->footer, &TextField::textChanged, this,
            [this] { emit footerChanged(d->footer->text()); });
    connect(d->printFooterOnTitlePage, &CheckBox::checkedChanged, this,
            &AudioplayParametersView::printFooterOnTitlePageChanged);
    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            &AudioplayParametersView::overrideCommonSettingsChanged);
    connect(d->audioplayTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto templateId
                    = _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
                emit audioplayTemplateChanged(templateId);
            });
    connect(d->showBlockNumbers, &CheckBox::checkedChanged, this,
            &AudioplayParametersView::showBlockNumbersChanged);
    connect(d->continueBlockNumbers, &CheckBox::checkedChanged, this,
            &AudioplayParametersView::continueBlockNumbersChanged);

    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this, [this](bool _checked) {
        d->audioplayTemplate->setVisible(_checked);
        d->showBlockNumbers->setVisible(_checked);
        // d->continueBlockNumbers->setVisible(_checked);
    });
    connect(d->showBlockNumbers, &CheckBox::checkedChanged, d->continueBlockNumbers,
            &CheckBox::setEnabled);
}

AudioplayParametersView::~AudioplayParametersView() = default;

QWidget* AudioplayParametersView::asQWidget()
{
    return this;
}

void AudioplayParametersView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->header->setReadOnly(readOnly);
    d->footer->setReadOnly(readOnly);
    d->audioplayTemplate->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->printHeaderOnTitlePage->setEnabled(enabled);
    d->printFooterOnTitlePage->setEnabled(enabled);
    d->overrideCommonSettings->setEnabled(enabled);
    d->showBlockNumbers->setEnabled(enabled);
    d->continueBlockNumbers->setEnabled(enabled);
}

void AudioplayParametersView::setHeader(const QString& _header)
{
    if (d->header->text() == _header) {
        return;
    }

    d->header->setText(_header);
}

void AudioplayParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printHeaderOnTitlePage->setChecked(_print);
}

void AudioplayParametersView::setFooter(const QString& _footer)
{
    if (d->footer->text() == _footer) {
        return;
    }

    d->footer->setText(_footer);
}

void AudioplayParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printFooterOnTitlePage->setChecked(_print);
}

void AudioplayParametersView::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings->isChecked() == _override) {
        return;
    }

    d->overrideCommonSettings->setChecked(_override);
}

void AudioplayParametersView::setAudioplayTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::audioplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::audioplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        if (d->audioplayTemplate->currentIndex() != item->index()) {
            d->audioplayTemplate->setCurrentIndex(item->index());
        }
        break;
    }
}

void AudioplayParametersView::setShowBlockNumbers(bool _show)
{
    if (d->showBlockNumbers->isChecked() == _show) {
        return;
    }

    d->showBlockNumbers->setChecked(_show);
}

void AudioplayParametersView::setContinueBlockNumbers(bool _continue)
{
    if (d->continueBlockNumbers->isChecked() == _continue) {
        return;
    }

    d->continueBlockNumbers->setChecked(_continue);
}

void AudioplayParametersView::updateTranslations()
{
    d->header->setLabel(tr("Header"));
    d->printHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->footer->setLabel(tr("Footer"));
    d->printFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->overrideCommonSettings->setText(tr("Override common settings for this audioplay"));
    d->audioplayTemplate->setLabel(tr("Template"));
    d->showBlockNumbers->setText(tr("Show block numbers"));
    d->continueBlockNumbers->setText(tr("Continue block numbers through document"));
}

void AudioplayParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->audioplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->header,
             d->footer,
             d->audioplayTemplate,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->audioplayTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto checkBox : {
             d->printHeaderOnTitlePage,
             d->printFooterOnTitlePage,
             d->overrideCommonSettings,
             d->showBlockNumbers,
             d->continueBlockNumbers,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->infoLayout->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->infoLayout->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                      static_cast<int>(Ui::DesignSystem::layout().px12()));
}

} // namespace Ui
