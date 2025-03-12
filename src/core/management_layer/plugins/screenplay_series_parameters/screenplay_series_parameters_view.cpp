#include "screenplay_series_parameters_view.h"

#include <business_layer/chronometry/chronometer.h>
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
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

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
    //
    RadioButton* screenplayDurationByPage = nullptr;
    TextField* screenplayDurationByPagePage = nullptr;
    TextField* screenplayDurationByPageDuration = nullptr;
    //
    RadioButton* screenplayDurationByCharacters = nullptr;
    TextField* screenplayDurationByCharactersCharacters = nullptr;
    CheckBox* screenplayDurationByCharactersIncludingSpaces = nullptr;
    TextField* screenplayDurationByCharactersDuration = nullptr;
    //
    RadioButton* screenplayDurationConfigurable = nullptr;
    Body1Label* screenplayDurationConfigurableForActionLabel = nullptr;
    TextField* screenplayDurationConfigurablePerParagraphForAction = nullptr;
    Body1Label* screenplayDurationConfigurableForActionPlus = nullptr;
    TextField* screenplayDurationConfigurablePerEvery50ForAction = nullptr;
    Body1Label* screenplayDurationConfigurableForDialogueLabel = nullptr;
    TextField* screenplayDurationConfigurablePerParagraphForDialogue = nullptr;
    Body1Label* screenplayDurationConfigurableForDialoguePlus = nullptr;
    TextField* screenplayDurationConfigurablePerEvery50ForDialogue = nullptr;
    Body1Label* screenplayDurationConfigurableForSceneHeadingLabel = nullptr;
    TextField* screenplayDurationConfigurablePerParagraphForSceneHeading = nullptr;
    Body1Label* screenplayDurationConfigurableForSceneHeadingPlus = nullptr;
    TextField* screenplayDurationConfigurablePerEvery50ForSceneHeading = nullptr;
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
    , screenplayDurationByPage(new RadioButton(seriesInfo))
    , screenplayDurationByPagePage(new TextField(seriesInfo))
    , screenplayDurationByPageDuration(new TextField(seriesInfo))
    , screenplayDurationByCharacters(new RadioButton(seriesInfo))
    , screenplayDurationByCharactersCharacters(new TextField(seriesInfo))
    , screenplayDurationByCharactersIncludingSpaces(new CheckBox(seriesInfo))
    , screenplayDurationByCharactersDuration(new TextField(seriesInfo))
    , screenplayDurationConfigurable(new RadioButton(seriesInfo))
    , screenplayDurationConfigurableForActionLabel(new Body1Label(seriesInfo))
    , screenplayDurationConfigurablePerParagraphForAction(new TextField(seriesInfo))
    , screenplayDurationConfigurableForActionPlus(new Body1Label(seriesInfo))
    , screenplayDurationConfigurablePerEvery50ForAction(new TextField(seriesInfo))
    , screenplayDurationConfigurableForDialogueLabel(new Body1Label(seriesInfo))
    , screenplayDurationConfigurablePerParagraphForDialogue(new TextField(seriesInfo))
    , screenplayDurationConfigurableForDialoguePlus(new Body1Label(seriesInfo))
    , screenplayDurationConfigurablePerEvery50ForDialogue(new TextField(seriesInfo))
    , screenplayDurationConfigurableForSceneHeadingLabel(new Body1Label(seriesInfo))
    , screenplayDurationConfigurablePerParagraphForSceneHeading(new TextField(seriesInfo))
    , screenplayDurationConfigurableForSceneHeadingPlus(new Body1Label(seriesInfo))
    , screenplayDurationConfigurablePerEvery50ForSceneHeading(new TextField(seriesInfo))
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
    auto durationGroup = new RadioButtonGroup(seriesInfo);
    durationGroup->add(screenplayDurationByPage);
    durationGroup->add(screenplayDurationByCharacters);
    durationGroup->add(screenplayDurationConfigurable);
    screenplayDurationByPage->setChecked(true);
    screenplayDurationByPage->setVisible(false);
    screenplayDurationByPagePage->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByPagePage->setText("1");
    screenplayDurationByPagePage->setReadOnly(true);
    screenplayDurationByPagePage->setVisible(false);
    screenplayDurationByPageDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByPageDuration->setVisible(false);
    screenplayDurationByCharacters->setVisible(false);
    screenplayDurationByCharactersCharacters->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersCharacters->setVisible(false);
    screenplayDurationByCharactersIncludingSpaces->setVisible(false);
    screenplayDurationByCharactersDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersDuration->setVisible(false);
    screenplayDurationConfigurable->setVisible(false);
    screenplayDurationConfigurableForActionLabel->setVisible(false);
    screenplayDurationConfigurablePerParagraphForAction->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerParagraphForAction->setVisible(false);
    screenplayDurationConfigurableForActionPlus->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForAction->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerEvery50ForAction->setVisible(false);
    screenplayDurationConfigurableForDialogueLabel->setVisible(false);
    screenplayDurationConfigurablePerParagraphForDialogue->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerParagraphForDialogue->setVisible(false);
    screenplayDurationConfigurablePerParagraphForDialogue->setLabelVisible(false);
    screenplayDurationConfigurableForDialoguePlus->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForDialogue->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerEvery50ForDialogue->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForDialogue->setLabelVisible(false);
    screenplayDurationConfigurableForSceneHeadingLabel->setVisible(false);
    screenplayDurationConfigurablePerParagraphForSceneHeading->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerParagraphForSceneHeading->setVisible(false);
    screenplayDurationConfigurablePerParagraphForSceneHeading->setLabelVisible(false);
    screenplayDurationConfigurableForSceneHeadingPlus->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForSceneHeading->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerEvery50ForSceneHeading->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForSceneHeading->setLabelVisible(false);


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
        auto layout = UiHelper::makeHBoxLayout();
        layout->addWidget(showSceneNumbers);
        layout->addWidget(showSceneNumbersOnLeft);
        layout->addWidget(showSceneNumbersOnRight);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(showDialoguesNumbers);
    infoLayout->addWidget(screenplayDurationByPage);
    {
        auto layout = UiHelper::makeHBoxLayout();
        layout->addWidget(screenplayDurationByPagePage);
        layout->addWidget(screenplayDurationByPageDuration);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(screenplayDurationByCharacters);
    {
        auto layout = UiHelper::makeHBoxLayout();
        layout->addWidget(screenplayDurationByCharactersCharacters);
        layout->addWidget(screenplayDurationByCharactersIncludingSpaces, 0, Qt::AlignCenter);
        layout->addWidget(screenplayDurationByCharactersDuration);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(screenplayDurationConfigurable);
    {
        auto layout = new QGridLayout;
        int row = 0;
        int column = 0;
        layout->addWidget(screenplayDurationConfigurableForActionLabel, row, column++,
                          Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(screenplayDurationConfigurablePerParagraphForAction, row, column++);
        layout->addWidget(screenplayDurationConfigurableForActionPlus, row, column++,
                          Qt::AlignCenter);
        layout->addWidget(screenplayDurationConfigurablePerEvery50ForAction, row, column++);
        ++row;
        column = 0;
        layout->addWidget(screenplayDurationConfigurableForDialogueLabel, row, column++,
                          Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(screenplayDurationConfigurablePerParagraphForDialogue, row, column++);
        layout->addWidget(screenplayDurationConfigurableForDialoguePlus, row, column++,
                          Qt::AlignCenter);
        layout->addWidget(screenplayDurationConfigurablePerEvery50ForDialogue, row, column++);
        ++row;
        column = 0;
        layout->addWidget(screenplayDurationConfigurableForSceneHeadingLabel, row, column++,
                          Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(screenplayDurationConfigurablePerParagraphForSceneHeading, row, column++);
        layout->addWidget(screenplayDurationConfigurableForSceneHeadingPlus, row, column++,
                          Qt::AlignCenter);
        layout->addWidget(screenplayDurationConfigurablePerEvery50ForSceneHeading, row, column++);
        layout->setColumnStretch(column, 1);
        infoLayout->addLayout(layout);
    }
    seriesInfo->setContentLayout(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(seriesInfo);
    layout->addStretch(1);
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
        if (!d->overrideCommonSettings->isVisibleTo(this)) {
            return;
        }

        for (auto widget : std::vector<QWidget*>{
                 d->screenplayTemplate,
                 d->showSceneNumbers,
                 d->showSceneNumbersOnLeft,
                 d->showSceneNumbersOnRight,
                 d->showDialoguesNumbers,
                 d->screenplayDurationByPage,
                 d->screenplayDurationByCharacters,
                 d->screenplayDurationConfigurable,
             }) {
            widget->setVisible(_checked);
        }

        if (d->overrideCommonSettings->isChecked()) {
            if (d->screenplayDurationByPage->isChecked()) {
                d->screenplayDurationByPagePage->setVisible(_checked);
                d->screenplayDurationByPageDuration->setVisible(_checked);
            } else if (d->screenplayDurationByCharacters->isChecked()) {
                d->screenplayDurationByCharactersCharacters->setVisible(_checked);
                d->screenplayDurationByCharactersIncludingSpaces->setVisible(_checked);
                d->screenplayDurationByCharactersDuration->setVisible(_checked);
            } else if (d->screenplayDurationConfigurable->isChecked()) {
                d->screenplayDurationConfigurableForActionLabel->setVisible(_checked);
                d->screenplayDurationConfigurablePerParagraphForAction->setVisible(_checked);
                d->screenplayDurationConfigurableForActionPlus->setVisible(_checked);
                d->screenplayDurationConfigurablePerEvery50ForAction->setVisible(_checked);
                d->screenplayDurationConfigurableForDialogueLabel->setVisible(_checked);
                d->screenplayDurationConfigurablePerParagraphForDialogue->setVisible(_checked);
                d->screenplayDurationConfigurableForDialoguePlus->setVisible(_checked);
                d->screenplayDurationConfigurablePerEvery50ForDialogue->setVisible(_checked);
                d->screenplayDurationConfigurableForSceneHeadingLabel->setVisible(_checked);
                d->screenplayDurationConfigurablePerParagraphForSceneHeading->setVisible(_checked);
                d->screenplayDurationConfigurableForSceneHeadingPlus->setVisible(_checked);
                d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setVisible(_checked);
            }
        } else {
            for (auto widget : std::vector<QWidget*>{
                     d->screenplayDurationByPage,
                     d->screenplayDurationByPagePage,
                     d->screenplayDurationByPageDuration,
                     d->screenplayDurationByCharacters,
                     d->screenplayDurationByCharactersCharacters,
                     d->screenplayDurationByCharactersIncludingSpaces,
                     d->screenplayDurationByCharactersDuration,
                     d->screenplayDurationConfigurable,
                     d->screenplayDurationConfigurableForActionLabel,
                     d->screenplayDurationConfigurablePerParagraphForAction,
                     d->screenplayDurationConfigurableForActionPlus,
                     d->screenplayDurationConfigurablePerEvery50ForAction,
                     d->screenplayDurationConfigurableForDialogueLabel,
                     d->screenplayDurationConfigurablePerParagraphForDialogue,
                     d->screenplayDurationConfigurableForDialoguePlus,
                     d->screenplayDurationConfigurablePerEvery50ForDialogue,
                     d->screenplayDurationConfigurableForSceneHeadingLabel,
                     d->screenplayDurationConfigurablePerParagraphForSceneHeading,
                     d->screenplayDurationConfigurableForSceneHeadingPlus,
                     d->screenplayDurationConfigurablePerEvery50ForSceneHeading,
                 }) {
                widget->setVisible(_checked);
            }
        }
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
    //
    // ... хронометраж
    //
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged, this, [this](bool _checked) {
        d->screenplayDurationByPagePage->setVisible(d->overrideCommonSettings->isChecked()
                                                    && _checked);
        d->screenplayDurationByPageDuration->setVisible(d->overrideCommonSettings->isChecked()
                                                        && _checked);
    });
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged, this,
            [this](bool _checked) {
                d->screenplayDurationByCharactersCharacters->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationByCharactersIncludingSpaces->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationByCharactersDuration->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
            });
    connect(d->screenplayDurationConfigurable, &RadioButton::checkedChanged, this,
            [this](bool _checked) {
                d->screenplayDurationConfigurableForActionLabel->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForAction->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForActionPlus->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForAction->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForDialogueLabel->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForDialogue->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForDialoguePlus->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForDialogue->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForSceneHeadingLabel->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForSceneHeading->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForSceneHeadingPlus->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setVisible(
                    d->overrideCommonSettings->isChecked() && _checked);
            });
    //
    auto notifyChronometerOptionsChanged = [this] {
        using namespace BusinessLayer;
        ChronometerOptions options;
        if (d->screenplayDurationByPage->isChecked()) {
            options.type = ChronometerType::Page;
        } else if (d->screenplayDurationByCharacters->isChecked()) {
            options.type = ChronometerType::Characters;
        } else if (d->screenplayDurationConfigurable->isChecked()) {
            options.type = ChronometerType::Sophocles;
        }

        options.page.seconds = d->screenplayDurationByPageDuration->text().toInt();

        options.characters.characters = d->screenplayDurationByCharactersCharacters->text().toInt();
        options.characters.considerSpaces
            = d->screenplayDurationByCharactersIncludingSpaces->isChecked();
        options.characters.seconds = d->screenplayDurationByCharactersDuration->text().toInt();

        options.sophocles.secsPerAction
            = d->screenplayDurationConfigurablePerParagraphForAction->text().toDouble();
        options.sophocles.secsPerEvery50Action
            = d->screenplayDurationConfigurablePerEvery50ForAction->text().toDouble();
        options.sophocles.secsPerDialogue
            = d->screenplayDurationConfigurablePerParagraphForDialogue->text().toDouble();
        options.sophocles.secsPerEvery50Dialogue
            = d->screenplayDurationConfigurablePerEvery50ForDialogue->text().toDouble();
        options.sophocles.secsPerSceneHeading
            = d->screenplayDurationConfigurablePerParagraphForSceneHeading->text().toDouble();
        options.sophocles.secsPerEvery50SceneHeading
            = d->screenplayDurationConfigurablePerEvery50ForSceneHeading->text().toDouble();

        emit chronometerOptionsChanged(options);
    };
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurable, &RadioButton::checkedChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationByPageDuration, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationByCharactersCharacters, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationByCharactersIncludingSpaces, &CheckBox::checkedChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationByCharactersDuration, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurablePerParagraphForAction, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurablePerEvery50ForAction, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurablePerParagraphForDialogue, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurablePerEvery50ForDialogue, &TextField::textChanged, this,
            notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurablePerParagraphForSceneHeading, &TextField::textChanged,
            this, notifyChronometerOptionsChanged);
    connect(d->screenplayDurationConfigurablePerEvery50ForSceneHeading, &TextField::textChanged,
            notifyChronometerOptionsChanged);
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
void ScreenplaySeriesParametersView::setChronometerOptions(
    const BusinessLayer::ChronometerOptions& _options)
{
    using namespace BusinessLayer;
    switch (_options.type) {
    default:
    case ChronometerType::Page: {
        d->screenplayDurationByPage->setChecked(true);
        break;
    }

    case ChronometerType::Characters: {
        d->screenplayDurationByCharacters->setChecked(true);
        break;
    }

    case ChronometerType::Sophocles: {
        d->screenplayDurationConfigurable->setChecked(true);
        break;
    }
    }

    d->screenplayDurationByPageDuration->setText(QString::number(_options.page.seconds));

    d->screenplayDurationByCharactersCharacters->setText(
        QString::number(_options.characters.characters));
    d->screenplayDurationByCharactersIncludingSpaces->setChecked(
        _options.characters.considerSpaces);
    d->screenplayDurationByCharactersDuration->setText(
        QString::number(_options.characters.seconds));

    d->screenplayDurationConfigurablePerParagraphForAction->setText(
        QString::number(_options.sophocles.secsPerAction, 'f', 1));
    d->screenplayDurationConfigurablePerEvery50ForAction->setText(
        QString::number(_options.sophocles.secsPerEvery50Action, 'f', 1));
    d->screenplayDurationConfigurablePerParagraphForDialogue->setText(
        QString::number(_options.sophocles.secsPerDialogue, 'f', 1));
    d->screenplayDurationConfigurablePerEvery50ForDialogue->setText(
        QString::number(_options.sophocles.secsPerEvery50Dialogue, 'f', 1));
    d->screenplayDurationConfigurablePerParagraphForSceneHeading->setText(
        QString::number(_options.sophocles.secsPerSceneHeading, 'f', 1));
    d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setText(
        QString::number(_options.sophocles.secsPerEvery50SceneHeading, 'f', 1));
}

void ScreenplaySeriesParametersView::updateTranslations()
{
    d->header->setLabel(tr("Header"));
    d->printHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->footer->setLabel(tr("Footer"));
    d->printFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->overrideCommonSettings->setText(tr("Override common settings for this series"));
    d->screenplayTemplate->setLabel(tr("Template"));
    d->showSceneNumbers->setText(tr("Print scenes numbers"));
    d->showSceneNumbersOnLeft->setText(tr("on the left"));
    d->showSceneNumbersOnRight->setText(tr("on the right"));
    d->showDialoguesNumbers->setText(tr("Print dialogues numbers"));
    d->screenplayDurationByPage->setText(tr("Calculate duration based on the count of pages"));
    d->screenplayDurationByPagePage->setLabel(tr("at the rate of"));
    d->screenplayDurationByPagePage->setSuffix(tr("pages"));
    d->screenplayDurationByPageDuration->setLabel(tr("has duration"));
    d->screenplayDurationByPageDuration->setSuffix(tr("seconds"));
    d->screenplayDurationByCharacters->setText(
        tr("Calculate duration based on the count of letters"));
    d->screenplayDurationByCharactersCharacters->setLabel(tr("at the rate of"));
    d->screenplayDurationByCharactersCharacters->setSuffix(tr("letters"));
    d->screenplayDurationByCharactersIncludingSpaces->setText(tr("including spaces"));
    d->screenplayDurationByCharactersDuration->setLabel(tr("has duration"));
    d->screenplayDurationByCharactersDuration->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurable->setText(tr("Calculate duration based on the custom rules"));
    d->screenplayDurationConfigurableForActionLabel->setText(tr("For action"));
    d->screenplayDurationConfigurablePerParagraphForAction->setLabel(tr("Per entry"));
    d->screenplayDurationConfigurablePerParagraphForAction->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForActionPlus->setText(tr("+"));
    d->screenplayDurationConfigurablePerEvery50ForAction->setLabel(tr("Each 50 characters"));
    d->screenplayDurationConfigurablePerEvery50ForAction->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForDialogueLabel->setText(tr("For dialogue"));
    d->screenplayDurationConfigurablePerParagraphForDialogue->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForDialoguePlus->setText(tr("+"));
    d->screenplayDurationConfigurablePerEvery50ForDialogue->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForSceneHeadingLabel->setText(tr("For scene heading"));
    d->screenplayDurationConfigurablePerParagraphForSceneHeading->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForSceneHeadingPlus->setText(tr("+"));
    d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setSuffix(tr("seconds"));
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
             d->screenplayDurationByPagePage,
             d->screenplayDurationByPageDuration,
             d->screenplayDurationByCharactersCharacters,
             d->screenplayDurationByCharactersDuration,
             d->screenplayDurationConfigurablePerParagraphForAction,
             d->screenplayDurationConfigurablePerEvery50ForAction,
             d->screenplayDurationConfigurablePerParagraphForDialogue,
             d->screenplayDurationConfigurablePerEvery50ForDialogue,
             d->screenplayDurationConfigurablePerParagraphForSceneHeading,
             d->screenplayDurationConfigurablePerEvery50ForSceneHeading,
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
             d->screenplayDurationByCharactersIncludingSpaces,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(static_cast<int>(Ui::DesignSystem::button().shadowMargins().top()));
    labelMargins.setBottom(static_cast<int>(Ui::DesignSystem::button().shadowMargins().bottom()));
    for (auto label : std::vector<Widget*>{
             d->screenplayDurationConfigurableForActionLabel,
             d->screenplayDurationConfigurableForActionPlus,
             d->screenplayDurationConfigurableForDialogueLabel,
             d->screenplayDurationConfigurableForDialoguePlus,
             d->screenplayDurationConfigurableForSceneHeadingLabel,
             d->screenplayDurationConfigurableForSceneHeadingPlus,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins(labelMargins);
    }

    for (auto radioButton : {
             d->screenplayDurationByPage,
             d->screenplayDurationByCharacters,
             d->screenplayDurationConfigurable,
         }) {
        radioButton->setBackgroundColor(DesignSystem::color().background());
        radioButton->setTextColor(DesignSystem::color().onBackground());
    }

    d->infoLayout->setSpacing(static_cast<int>(Ui::DesignSystem::compactLayout().px16()));
    d->infoLayout->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                      static_cast<int>(Ui::DesignSystem::layout().px12()));
}

} // namespace Ui
