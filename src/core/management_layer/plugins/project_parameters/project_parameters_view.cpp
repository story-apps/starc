#include "project_parameters_view.h"

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

class ProjectParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* projectInfo = nullptr;
    QVBoxLayout* infoLayout = nullptr;

    CheckBox* overrideScreenplaySettings = nullptr;
    ComboBox* screenplayEditorTemplate = nullptr;
    CheckBox* screenplayEditorShowSceneNumbers = nullptr;
    CheckBox* screenplayEditorShowSceneNumbersOnLeft = nullptr;
    CheckBox* screenplayEditorShowSceneNumbersOnRight = nullptr;
    CheckBox* screenplayEditorShowDialoguesNumbers = nullptr;
    CheckBox* screenplayEditorCorrectSceneCharacters = nullptr;
    CheckBox* screenplayEditorCorrectTextOnPageBreaks = nullptr;
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

ProjectParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , projectInfo(new Card(_parent))
    , infoLayout(new QVBoxLayout)
    , overrideScreenplaySettings(new CheckBox(projectInfo))
    , screenplayEditorTemplate(new ComboBox(projectInfo))
    , screenplayEditorShowSceneNumbers(new CheckBox(projectInfo))
    , screenplayEditorShowSceneNumbersOnLeft(new CheckBox(projectInfo))
    , screenplayEditorShowSceneNumbersOnRight(new CheckBox(projectInfo))
    , screenplayEditorShowDialoguesNumbers(new CheckBox(projectInfo))
    , screenplayEditorCorrectSceneCharacters(new CheckBox(projectInfo))
    , screenplayEditorCorrectTextOnPageBreaks(new CheckBox(projectInfo))
    , screenplayDurationByPage(new RadioButton(projectInfo))
    , screenplayDurationByPagePage(new TextField(projectInfo))
    , screenplayDurationByPageDuration(new TextField(projectInfo))
    , screenplayDurationByCharacters(new RadioButton(projectInfo))
    , screenplayDurationByCharactersCharacters(new TextField(projectInfo))
    , screenplayDurationByCharactersIncludingSpaces(new CheckBox(projectInfo))
    , screenplayDurationByCharactersDuration(new TextField(projectInfo))
    , screenplayDurationConfigurable(new RadioButton(projectInfo))
    , screenplayDurationConfigurableForActionLabel(new Body1Label(projectInfo))
    , screenplayDurationConfigurablePerParagraphForAction(new TextField(projectInfo))
    , screenplayDurationConfigurableForActionPlus(new Body1Label(projectInfo))
    , screenplayDurationConfigurablePerEvery50ForAction(new TextField(projectInfo))
    , screenplayDurationConfigurableForDialogueLabel(new Body1Label(projectInfo))
    , screenplayDurationConfigurablePerParagraphForDialogue(new TextField(projectInfo))
    , screenplayDurationConfigurableForDialoguePlus(new Body1Label(projectInfo))
    , screenplayDurationConfigurablePerEvery50ForDialogue(new TextField(projectInfo))
    , screenplayDurationConfigurableForSceneHeadingLabel(new Body1Label(projectInfo))
    , screenplayDurationConfigurablePerParagraphForSceneHeading(new TextField(projectInfo))
    , screenplayDurationConfigurableForSceneHeadingPlus(new Body1Label(projectInfo))
    , screenplayDurationConfigurablePerEvery50ForSceneHeading(new TextField(projectInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    screenplayEditorTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayEditorTemplate->setModel(BusinessLayer::TemplatesFacade::screenplayTemplates());
    screenplayEditorTemplate->hide();

    screenplayEditorShowSceneNumbers->hide();
    screenplayEditorShowSceneNumbersOnLeft->setEnabled(false);
    screenplayEditorShowSceneNumbersOnLeft->hide();
    screenplayEditorShowSceneNumbersOnRight->setEnabled(false);
    screenplayEditorShowSceneNumbersOnRight->hide();
    screenplayEditorShowDialoguesNumbers->hide();
    screenplayEditorCorrectSceneCharacters->hide();
    screenplayEditorCorrectTextOnPageBreaks->hide();
    auto durationGroup = new RadioButtonGroup(projectInfo);
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
    infoLayout->addWidget(overrideScreenplaySettings, 1, Qt::AlignTop);
    infoLayout->addWidget(screenplayEditorTemplate);
    {
        auto layout = UiHelper::makeHBoxLayout();
        layout->addWidget(screenplayEditorShowSceneNumbers);
        layout->addWidget(screenplayEditorShowSceneNumbersOnLeft);
        layout->addWidget(screenplayEditorShowSceneNumbersOnRight);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(screenplayEditorShowDialoguesNumbers);
    infoLayout->addWidget(screenplayEditorCorrectSceneCharacters);
    infoLayout->addWidget(screenplayEditorCorrectTextOnPageBreaks);
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
    projectInfo->setContentLayout(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(projectInfo);
    layout->addStretch(1);
    contentWidget->setLayout(layout);
}


// ****


ProjectParametersView::ProjectParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->overrideScreenplaySettings, &CheckBox::checkedChanged, this,
            &ProjectParametersView::overrideScreenplaySettingsChanged);
    connect(d->screenplayEditorTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto templateId
                    = _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
                emit screenplayEditorTemplateChanged(templateId);
            });
    connect(d->screenplayEditorShowSceneNumbers, &CheckBox::checkedChanged, this,
            &ProjectParametersView::screenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowSceneNumbersOnLeft, &CheckBox::checkedChanged, this,
            &ProjectParametersView::screenplayEditorShowSceneNumbersOnLeftChanged);
    connect(d->screenplayEditorShowSceneNumbersOnRight, &CheckBox::checkedChanged, this,
            &ProjectParametersView::screenplayEditorShowSceneNumbersOnRightChanged);
    connect(d->screenplayEditorShowDialoguesNumbers, &CheckBox::checkedChanged, this,
            &ProjectParametersView::screenplayEditorShowDialoguesNumbersChanged);
    connect(d->screenplayEditorCorrectSceneCharacters, &CheckBox::checkedChanged, this,
            &ProjectParametersView::screenplayEditorCorrectSceneCharactersChanged);
    connect(d->screenplayEditorCorrectTextOnPageBreaks, &CheckBox::checkedChanged, this,
            &ProjectParametersView::screenplayEditorCorrectTextOnPageBreaksChanged);

    connect(d->overrideScreenplaySettings, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (!d->overrideScreenplaySettings->isVisibleTo(this)) {
            return;
        }

        for (auto widget : std::vector<QWidget*>{
                 d->screenplayEditorTemplate,
                 d->screenplayEditorShowSceneNumbers,
                 d->screenplayEditorShowSceneNumbersOnLeft,
                 d->screenplayEditorShowSceneNumbersOnRight,
                 d->screenplayEditorShowDialoguesNumbers,
                 d->screenplayEditorCorrectSceneCharacters,
                 d->screenplayEditorCorrectTextOnPageBreaks,
                 d->screenplayDurationByPage,
                 d->screenplayDurationByCharacters,
                 d->screenplayDurationConfigurable,
             }) {
            widget->setVisible(_checked);
        }

        if (d->overrideScreenplaySettings->isChecked()) {
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
    connect(d->screenplayEditorShowSceneNumbers, &CheckBox::checkedChanged,
            d->screenplayEditorShowSceneNumbersOnLeft, &CheckBox::setEnabled);
    connect(d->screenplayEditorShowSceneNumbers, &CheckBox::checkedChanged,
            d->screenplayEditorShowSceneNumbersOnRight, &CheckBox::setEnabled);
    auto correctShownSceneNumber = [this] {
        if (!d->screenplayEditorShowSceneNumbersOnLeft->isChecked()
            && !d->screenplayEditorShowSceneNumbersOnRight->isChecked()) {
            d->screenplayEditorShowSceneNumbersOnLeft->setChecked(true);
        }
    };
    connect(d->screenplayEditorShowSceneNumbersOnLeft, &CheckBox::checkedChanged, this,
            correctShownSceneNumber);
    connect(d->screenplayEditorShowSceneNumbersOnRight, &CheckBox::checkedChanged, this,
            correctShownSceneNumber);
    //
    // ... хронометраж
    //
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged, this, [this](bool _checked) {
        d->screenplayDurationByPagePage->setVisible(d->overrideScreenplaySettings->isChecked()
                                                    && _checked);
        d->screenplayDurationByPageDuration->setVisible(d->overrideScreenplaySettings->isChecked()
                                                        && _checked);
    });
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged, this,
            [this](bool _checked) {
                d->screenplayDurationByCharactersCharacters->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationByCharactersIncludingSpaces->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationByCharactersDuration->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
            });
    connect(d->screenplayDurationConfigurable, &RadioButton::checkedChanged, this,
            [this](bool _checked) {
                d->screenplayDurationConfigurableForActionLabel->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForAction->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForActionPlus->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForAction->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForDialogueLabel->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForDialogue->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForDialoguePlus->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForDialogue->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForSceneHeadingLabel->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForSceneHeading->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurableForSceneHeadingPlus->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setVisible(
                    d->overrideScreenplaySettings->isChecked() && _checked);
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

        emit screenplayDurationOptionsChanged(options);
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

ProjectParametersView::~ProjectParametersView() = default;

QWidget* ProjectParametersView::asQWidget()
{
    return this;
}

void ProjectParametersView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->screenplayEditorTemplate->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->overrideScreenplaySettings->setEnabled(enabled);
    d->screenplayEditorShowSceneNumbers->setEnabled(enabled);
    d->screenplayEditorShowSceneNumbersOnLeft->setEnabled(enabled);
    d->screenplayEditorShowSceneNumbersOnRight->setEnabled(enabled);
    d->screenplayEditorShowDialoguesNumbers->setEnabled(enabled);
    d->screenplayEditorCorrectSceneCharacters->setEnabled(enabled);
    d->screenplayEditorCorrectTextOnPageBreaks->setEnabled(enabled);
    d->screenplayDurationByPage->setEnabled(enabled);
    d->screenplayDurationByPage->setEnabled(enabled);
    d->screenplayDurationByPagePage->setEnabled(enabled);
    d->screenplayDurationByPageDuration->setEnabled(enabled);
    d->screenplayDurationByCharacters->setEnabled(enabled);
    d->screenplayDurationByCharactersCharacters->setEnabled(enabled);
    d->screenplayDurationByCharactersIncludingSpaces->setEnabled(enabled);
    d->screenplayDurationByCharactersDuration->setEnabled(enabled);
    d->screenplayDurationConfigurable->setEnabled(enabled);
    d->screenplayDurationConfigurablePerParagraphForAction->setEnabled(enabled);
    d->screenplayDurationConfigurablePerEvery50ForAction->setEnabled(enabled);
    d->screenplayDurationConfigurablePerParagraphForDialogue->setEnabled(enabled);
    d->screenplayDurationConfigurablePerEvery50ForDialogue->setEnabled(enabled);
    d->screenplayDurationConfigurablePerParagraphForSceneHeading->setEnabled(enabled);
    d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setEnabled(enabled);
}

void ProjectParametersView::setOverrideScreenplaySettings(bool _override)
{
    if (d->overrideScreenplaySettings->isChecked() == _override) {
        return;
    }

    d->overrideScreenplaySettings->setChecked(_override);
}

void ProjectParametersView::setScreenplayEditorTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::screenplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::screenplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        if (d->screenplayEditorTemplate->currentIndex() != item->index()) {
            d->screenplayEditorTemplate->setCurrentIndex(item->index());
        }
        break;
    }
}

void ProjectParametersView::setScreenplayEditorShowSceneNumbers(bool _show)
{
    if (d->screenplayEditorShowSceneNumbers->isChecked() == _show) {
        return;
    }

    d->screenplayEditorShowSceneNumbers->setChecked(_show);
}

void ProjectParametersView::setScreenplayEditorShowSceneNumbersOnLeft(bool _show)
{
    if (d->screenplayEditorShowSceneNumbersOnLeft->isChecked() == _show) {
        return;
    }

    d->screenplayEditorShowSceneNumbersOnLeft->setChecked(_show);
}

void ProjectParametersView::setScreenplayEditorShowSceneNumbersOnRight(bool _show)
{
    if (d->screenplayEditorShowSceneNumbersOnRight->isChecked() == _show) {
        return;
    }

    d->screenplayEditorShowSceneNumbersOnRight->setChecked(_show);
}

void ProjectParametersView::setScreenplayEditorShowDialoguesNumbers(bool _show)
{
    if (d->screenplayEditorShowDialoguesNumbers->isChecked() == _show) {
        return;
    }

    d->screenplayEditorShowDialoguesNumbers->setChecked(_show);
}

void ProjectParametersView::setScreenplayEditorCorrectSceneCharacters(bool _correct)
{
    if (d->screenplayEditorCorrectSceneCharacters->isChecked() == _correct) {
        return;
    }

    d->screenplayEditorCorrectSceneCharacters->setChecked(_correct);
}

void ProjectParametersView::setScreenplayEditorCorrectTextOnPageBreaks(bool _correct)
{
    if (d->screenplayEditorCorrectTextOnPageBreaks->isChecked() == _correct) {
        return;
    }

    d->screenplayEditorCorrectTextOnPageBreaks->setChecked(_correct);
}

void ProjectParametersView::setScreenplayDurationOptions(
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

    auto setTextFor = [](TextField* _editor, const QString& _text) {
        if (_editor->text() != _text) {
            _editor->setText(_text);
        }
    };

    setTextFor(d->screenplayDurationByCharactersCharacters,
               QString::number(_options.characters.characters));
    d->screenplayDurationByCharactersIncludingSpaces->setChecked(
        _options.characters.considerSpaces);
    setTextFor(d->screenplayDurationByCharactersDuration,
               QString::number(_options.characters.seconds));

    setTextFor(d->screenplayDurationConfigurablePerParagraphForAction,
               QString::number(_options.sophocles.secsPerAction, 'f', 1));
    setTextFor(d->screenplayDurationConfigurablePerEvery50ForAction,
               QString::number(_options.sophocles.secsPerEvery50Action, 'f', 1));
    setTextFor(d->screenplayDurationConfigurablePerParagraphForDialogue,
               QString::number(_options.sophocles.secsPerDialogue, 'f', 1));
    setTextFor(d->screenplayDurationConfigurablePerEvery50ForDialogue,
               QString::number(_options.sophocles.secsPerEvery50Dialogue, 'f', 1));
    setTextFor(d->screenplayDurationConfigurablePerParagraphForSceneHeading,
               QString::number(_options.sophocles.secsPerSceneHeading, 'f', 1));
    setTextFor(d->screenplayDurationConfigurablePerEvery50ForSceneHeading,
               QString::number(_options.sophocles.secsPerEvery50SceneHeading, 'f', 1));
}

void ProjectParametersView::updateTranslations()
{
    d->overrideScreenplaySettings->setText(tr("Override common settings for screenplays"));
    d->screenplayEditorTemplate->setLabel(tr("Template"));
    d->screenplayEditorShowSceneNumbers->setText(tr("Print scenes numbers"));
    d->screenplayEditorShowSceneNumbersOnLeft->setText(tr("on the left"));
    d->screenplayEditorShowSceneNumbersOnRight->setText(tr("on the right"));
    d->screenplayEditorShowDialoguesNumbers->setText(tr("Print dialogues numbers"));
    d->screenplayEditorCorrectSceneCharacters->setText(
        tr("Automatically fill scene characters list"));
    d->screenplayEditorCorrectTextOnPageBreaks->setText(
        tr("Automatically correct screenplay text on page breaks"));
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

void ProjectParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->projectInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->screenplayEditorTemplate,
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
             d->screenplayEditorTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto checkBox : {
             d->overrideScreenplaySettings,
             d->screenplayEditorShowSceneNumbers,
             d->screenplayEditorShowSceneNumbersOnLeft,
             d->screenplayEditorShowSceneNumbersOnRight,
             d->screenplayEditorShowDialoguesNumbers,
             d->screenplayEditorCorrectSceneCharacters,
             d->screenplayEditorCorrectTextOnPageBreaks,
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
