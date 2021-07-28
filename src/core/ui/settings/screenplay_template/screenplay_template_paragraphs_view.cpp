#include "screenplay_template_paragraphs_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <utils/helpers/ui_helper.h>

#include <QFontDatabase>
#include <QScrollArea>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

class ScreenplayTemplateParagraphsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QScrollArea* content = nullptr;

    Card* card = nullptr;
    QVBoxLayout* cardLayout = nullptr;
    TabBar* paragraphs = nullptr;
    CheckBox* paragraphEnabled = nullptr;
    ComboBox* fontFamily = nullptr;
    QStringListModel* fontFamilyModel = nullptr;
    ComboBox* fontSize = nullptr;
    QStringListModel* fontSizeModel = nullptr;
    CheckBox* startsFromNewPage = nullptr;
    CheckBox* uppercase = nullptr;
    CheckBox* bold = nullptr;
    CheckBox* italic = nullptr;
    CheckBox* underline = nullptr;
    CaptionLabel* textAlignmentTitle = nullptr;
    RadioButton* alignLeft = nullptr;
    RadioButton* alignCenter = nullptr;
    RadioButton* alignRight = nullptr;
    CaptionLabel* verticalIndentationTitle = nullptr;
    TextField* topIndent = nullptr;
    TextField* bottomIndent = nullptr;
    RadioButton* verticalIndentationInLines = nullptr;
    RadioButton* verticalIndentationInMm = nullptr;
    CaptionLabel* horizontalIndentationTitle = nullptr;
    TextField* leftIndent = nullptr;
    TextField* rightIndent = nullptr;
    CaptionLabel* horizontalIndentationInTableTitle = nullptr;
    TextField* leftIndentInTable = nullptr;
    TextField* rightIndentInTable = nullptr;
    CaptionLabel* lineSpacingTitle = nullptr;
    ComboBox* lineSpacing = nullptr;
    QStringListModel* lineSpacingModel = nullptr;
    TextField* lineSpacingValue = nullptr;
    QVector<int> additionalSpacingRows;
};

ScreenplayTemplateParagraphsView::Implementation::Implementation(QWidget* _parent)
    : content(UiHelper::createScrollArea(_parent))
    , card(new Card(content))
    , cardLayout(new QVBoxLayout)
    , paragraphs(new TabBar(card))
    , paragraphEnabled(new CheckBox(card))
    , fontFamily(new ComboBox(card))
    , fontFamilyModel(new QStringListModel(fontFamily))
    , fontSize(new ComboBox(card))
    , fontSizeModel(new QStringListModel(fontSize))
    , startsFromNewPage(new CheckBox(card))
    , uppercase(new CheckBox(card))
    , bold(new CheckBox(card))
    , italic(new CheckBox(card))
    , underline(new CheckBox(card))
    , textAlignmentTitle(new CaptionLabel(card))
    , alignLeft(new RadioButton(card))
    , alignCenter(new RadioButton(card))
    , alignRight(new RadioButton(card))
    , verticalIndentationTitle(new CaptionLabel(card))
    , topIndent(new TextField(card))
    , bottomIndent(new TextField(card))
    , verticalIndentationInLines(new RadioButton(card))
    , verticalIndentationInMm(new RadioButton(card))
    , horizontalIndentationTitle(new CaptionLabel(card))
    , leftIndent(new TextField(card))
    , rightIndent(new TextField(card))
    , horizontalIndentationInTableTitle(new CaptionLabel(card))
    , leftIndentInTable(new TextField(card))
    , rightIndentInTable(new TextField(card))
    , lineSpacingTitle(new CaptionLabel(card))
    , lineSpacing(new ComboBox(card))
    , lineSpacingModel(new QStringListModel(lineSpacing))
    , lineSpacingValue(new TextField(card))

{
    for (int i = 0; i < 13; ++i) {
        paragraphs->addTab({});
    }

    fontFamily->setModel(fontFamilyModel);
    fontFamilyModel->setStringList(QFontDatabase().families());
    fontSize->setModel(fontSizeModel);
    fontSizeModel->setStringList(
        { "8", "9", "10", "11", "12", "14", "18", "24", "30", "36", "48", "60", "72", "96" });

    lineSpacing->setModel(lineSpacingModel);
    lineSpacingValue->setEnabled(false);

    cardLayout->setContentsMargins({});
    cardLayout->setSpacing(0);
    cardLayout->addWidget(paragraphs);
    additionalSpacingRows.append(cardLayout->count());
    cardLayout->addSpacing(0);
    cardLayout->addWidget(paragraphEnabled);
    additionalSpacingRows.append(cardLayout->count());
    cardLayout->addSpacing(0);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(fontFamily, 4);
        layout->addWidget(fontSize, 1);
        cardLayout->addLayout(layout);
    }
    additionalSpacingRows.append(cardLayout->count());
    cardLayout->addSpacing(0);
    cardLayout->addWidget(startsFromNewPage);
    cardLayout->addWidget(uppercase);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(bold);
        layout->addWidget(italic);
        layout->addWidget(underline);
        layout->addStretch();
        cardLayout->addLayout(layout);
    }
    cardLayout->addWidget(textAlignmentTitle);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(alignLeft);
        layout->addWidget(alignCenter);
        layout->addWidget(alignRight);
        layout->addStretch();
        cardLayout->addLayout(layout);
    }
    cardLayout->addWidget(verticalIndentationTitle);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(topIndent);
        layout->addWidget(bottomIndent);
        layout->addWidget(verticalIndentationInLines);
        layout->addWidget(verticalIndentationInMm);
        layout->addStretch();
        cardLayout->addLayout(layout);
    }
    cardLayout->addWidget(horizontalIndentationTitle);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(leftIndent);
        layout->addWidget(rightIndent);
        layout->addStretch();
        cardLayout->addLayout(layout);
    }
    cardLayout->addWidget(horizontalIndentationInTableTitle);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(leftIndentInTable);
        layout->addWidget(rightIndentInTable);
        layout->addStretch();
        cardLayout->addLayout(layout);
    }
    cardLayout->addWidget(lineSpacingTitle);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(lineSpacing);
        layout->addWidget(lineSpacingValue);
        layout->addStretch();
        cardLayout->addLayout(layout);
    }
    card->setLayoutReimpl(cardLayout);

    qobject_cast<QVBoxLayout*>(content->widget()->layout())->insertWidget(0, card);
}


// ****


ScreenplayTemplateParagraphsView::ScreenplayTemplateParagraphsView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTemplateParagraphsView::~ScreenplayTemplateParagraphsView() = default;

void ScreenplayTemplateParagraphsView::updateTranslations()
{
    int tab = 0;
    d->paragraphs->setTabName(tab++, tr("Scene heading"));
    d->paragraphs->setTabName(tab++, tr("Scene characters"));
    d->paragraphs->setTabName(tab++, tr("Action"));
    d->paragraphs->setTabName(tab++, tr("Character"));
    d->paragraphs->setTabName(tab++, tr("Parenthetical"));
    d->paragraphs->setTabName(tab++, tr("Dialogue"));
    d->paragraphs->setTabName(tab++, tr("Lyrics"));
    d->paragraphs->setTabName(tab++, tr("Shot"));
    d->paragraphs->setTabName(tab++, tr("Transition"));
    d->paragraphs->setTabName(tab++, tr("Inline note"));
    d->paragraphs->setTabName(tab++, tr("Unformatted text"));
    d->paragraphs->setTabName(tab++, tr("Folder header"));
    d->paragraphs->setTabName(tab++, tr("Folder footer"));
    d->paragraphEnabled->setText(tr("Is paragraph style available"));
    d->fontFamily->setLabel(tr("Font family"));
    d->fontSize->setLabel(tr("Font size"));
    d->startsFromNewPage->setText(tr("Place paragraph at the top of the page"));
    d->uppercase->setText(tr("Use UPPERCASE characters for paragraph text"));
    d->bold->setText(tr("Bold"));
    d->italic->setText(tr("Italic"));
    d->underline->setText(tr("Underline"));
    d->textAlignmentTitle->setText(tr("Align text on the page"));
    d->alignLeft->setText(tr("Left"));
    d->alignCenter->setText(tr("Center"));
    d->alignRight->setText(tr("Right"));
    d->verticalIndentationTitle->setText(tr("Vertical indentation"));
    d->topIndent->setLabel(tr("Top"));
    d->bottomIndent->setLabel(tr("Bottom"));
    d->verticalIndentationInLines->setText(tr("lines"));
    d->verticalIndentationInMm->setText(tr("mm"));
    d->horizontalIndentationTitle->setText(tr("Horizontal indentation"));
    d->leftIndent->setLabel(tr("Left"));
    d->leftIndent->setSuffix(tr("mm"));
    d->rightIndent->setLabel(tr("Right"));
    d->rightIndent->setSuffix(tr("mm"));
    d->horizontalIndentationInTableTitle->setText(
        tr("Horizontal indentation (for two-column mode)"));
    d->leftIndentInTable->setLabel(tr("Left"));
    d->leftIndentInTable->setSuffix(tr("mm"));
    d->rightIndentInTable->setLabel(tr("Right"));
    d->rightIndentInTable->setSuffix(tr("mm"));
    d->lineSpacingTitle->setText(tr("Line spacing"));
    d->lineSpacing->setLabel(tr("Type"));
    d->lineSpacingModel->setStringList(
        { tr("Single"), tr("One and half"), tr("Double"), tr("Fixed") });
    d->lineSpacingValue->setLabel(tr("Value"));
    d->lineSpacingValue->setSuffix(tr("mm"));
}

void ScreenplayTemplateParagraphsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().background());
    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());
    d->card->setBackgroundColor(Ui::DesignSystem::color().background());
    d->card->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0,
                                Ui::DesignSystem::layout().px24());

    for (auto row : d->additionalSpacingRows) {
        d->cardLayout->itemAt(row)->spacerItem()->changeSize(0, Ui::DesignSystem::layout().px8());
    }

    for (auto widget : std::vector<Widget*>{
             d->paragraphs, d->paragraphEnabled, d->startsFromNewPage, d->uppercase, d->bold,
             d->italic, d->underline, d->textAlignmentTitle, d->alignLeft, d->alignCenter,
             d->alignRight, d->verticalIndentationTitle, d->verticalIndentationInLines,
             d->verticalIndentationInMm, d->horizontalIndentationTitle,
             d->horizontalIndentationInTableTitle, d->lineSpacingTitle }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().background());
        widget->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->textAlignmentTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px4());
    d->verticalIndentationTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16());
    d->horizontalIndentationTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16());
    d->horizontalIndentationInTableTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12());
    d->lineSpacingTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12());

    for (auto textField :
         std::vector<TextField*>{ d->fontFamily, d->fontSize, d->topIndent, d->bottomIndent,
                                  d->leftIndent, d->rightIndent, d->leftIndentInTable,
                                  d->rightIndentInTable, d->lineSpacing, d->lineSpacingValue }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setMinimumWidth(Ui::DesignSystem::layout().px62() * 3);
    }
    for (auto textField : std::vector<TextField*>{
             d->fontFamily, d->topIndent, d->bottomIndent, d->leftIndent, d->rightIndent,
             d->leftIndentInTable, d->rightIndentInTable, d->lineSpacing, d->lineSpacingValue }) {
        textField->setCustomMargins(
            { isLeftToRight() ? Ui::DesignSystem::layout().px24() : 0.0, 0.0,
              isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px24(), 0.0 });
    }
}

} // namespace Ui
