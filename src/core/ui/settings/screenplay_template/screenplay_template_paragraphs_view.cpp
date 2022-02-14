#include "screenplay_template_paragraphs_view.h"

#include <business_layer/templates/screenplay_template.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QFontDatabase>
#include <QScrollArea>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
const QVector<BusinessLayer::ScreenplayParagraphType> kParagraphTypes
    = { BusinessLayer::ScreenplayParagraphType::SceneHeading,
        BusinessLayer::ScreenplayParagraphType::SceneCharacters,
        BusinessLayer::ScreenplayParagraphType::Action,
        BusinessLayer::ScreenplayParagraphType::Character,
        BusinessLayer::ScreenplayParagraphType::Parenthetical,
        BusinessLayer::ScreenplayParagraphType::Dialogue,
        BusinessLayer::ScreenplayParagraphType::Lyrics,
        BusinessLayer::ScreenplayParagraphType::Transition,
        BusinessLayer::ScreenplayParagraphType::Shot,
        BusinessLayer::ScreenplayParagraphType::InlineNote,
        BusinessLayer::ScreenplayParagraphType::UnformattedText,
        BusinessLayer::ScreenplayParagraphType::FolderHeader,
        BusinessLayer::ScreenplayParagraphType::FolderFooter };
}

class ScreenplayTemplateParagraphsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    bool useMm = true;

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
    RadioButton* alignJustify = nullptr;
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
    , alignJustify(new RadioButton(card))
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
    for (int i = 0; i < kParagraphTypes.size(); ++i) {
        paragraphs->addTab({});
    }

    fontFamily->setModel(fontFamilyModel);
    fontFamilyModel->setStringList(QFontDatabase().families());
    fontSize->setModel(fontSizeModel);
    fontSizeModel->setStringList(
        { "8", "9", "10", "11", "12", "14", "18", "24", "30", "36", "48", "60", "72", "96" });

    auto alignmentGroup = new RadioButtonGroup(card);
    alignmentGroup->add(alignLeft);
    alignmentGroup->add(alignCenter);
    alignmentGroup->add(alignRight);
    alignmentGroup->add(alignJustify);

    auto verticalIndentTypeGroup = new RadioButtonGroup(card);
    verticalIndentTypeGroup->add(verticalIndentationInLines);
    verticalIndentTypeGroup->add(verticalIndentationInMm);

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
        layout->addWidget(alignJustify);
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

    connect(d->paragraphs, &TabBar::currentIndexChanged, this,
            [this](int _currentIndex, int _previousIndex) {
                emit currentParagraphTypeChanged(kParagraphTypes.at(_currentIndex),
                                                 kParagraphTypes.at(_previousIndex));
            });
    connect(d->paragraphEnabled, &CheckBox::checkedChanged, this, [this](bool _enabled) {
        for (auto widget : std::vector<QWidget*>{
                 d->fontFamily,
                 d->fontSize,
                 d->startsFromNewPage,
                 d->uppercase,
                 d->bold,
                 d->italic,
                 d->underline,
                 d->textAlignmentTitle,
                 d->alignLeft,
                 d->alignCenter,
                 d->alignRight,
                 d->alignJustify,
                 d->verticalIndentationTitle,
                 d->topIndent,
                 d->bottomIndent,
                 d->verticalIndentationInLines,
                 d->verticalIndentationInMm,
                 d->horizontalIndentationTitle,
                 d->leftIndent,
                 d->rightIndent,
                 d->horizontalIndentationInTableTitle,
                 d->leftIndentInTable,
                 d->rightIndentInTable,
                 d->lineSpacingTitle,
                 d->lineSpacing,
             }) {
            widget->setEnabled(_enabled);
        }
        d->lineSpacingValue->setEnabled(
            d->paragraphEnabled->isChecked()
            && (d->lineSpacing->currentIndex().row() == (d->lineSpacingModel->rowCount() - 1)));
    });
    connect(d->lineSpacing, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                d->lineSpacingValue->setEnabled(
                    d->paragraphEnabled->isChecked()
                    && (_index.row() == (d->lineSpacingModel->rowCount() - 1)));
            });

    for (auto checkBox : std::vector<CheckBox*>{
             d->paragraphEnabled,
             d->startsFromNewPage,
             d->uppercase,
             d->bold,
             d->italic,
             d->underline,
         }) {
        connect(checkBox, &CheckBox::checkedChanged, this,
                &ScreenplayTemplateParagraphsView::currentParagraphChanged);
    }
    for (auto radioButton : std::vector<RadioButton*>{
             d->alignLeft,
             d->alignCenter,
             d->alignRight,
             d->alignJustify,
             d->verticalIndentationInLines,
             d->verticalIndentationInMm,
         }) {
        connect(radioButton, &RadioButton::checkedChanged, this,
                &ScreenplayTemplateParagraphsView::currentParagraphChanged);
    }
    for (auto textField : std::vector<TextField*>{
             d->fontFamily,
             d->fontSize,
             d->topIndent,
             d->bottomIndent,
             d->leftIndent,
             d->rightIndent,
             d->leftIndentInTable,
             d->rightIndentInTable,
             d->lineSpacing,
             d->lineSpacingValue,
         }) {
        connect(textField, &TextField::textChanged, this,
                &ScreenplayTemplateParagraphsView::currentParagraphChanged);
    }


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTemplateParagraphsView::~ScreenplayTemplateParagraphsView() = default;

void ScreenplayTemplateParagraphsView::setUseMm(bool _mm)
{
    if (d->useMm == _mm) {
        return;
    }

    d->useMm = _mm;
    updateTranslations();
}

BusinessLayer::ScreenplayParagraphType ScreenplayTemplateParagraphsView::currentParagraphType()
    const
{
    return kParagraphTypes.at(d->paragraphs->currentTab());
}

void ScreenplayTemplateParagraphsView::setCurrentParagraphType(
    BusinessLayer::ScreenplayParagraphType _type)
{
    d->paragraphs->setCurrentTab(kParagraphTypes.indexOf(_type));
}

bool ScreenplayTemplateParagraphsView::isParagraphEnabled() const
{
    return d->paragraphEnabled->isChecked();
}

void ScreenplayTemplateParagraphsView::setParagraphEnabled(bool _enabled)
{
    d->paragraphEnabled->setChecked(_enabled);
}

QString ScreenplayTemplateParagraphsView::fontFamily() const
{
    return d->fontFamily->text();
}

void ScreenplayTemplateParagraphsView::setFontFamily(const QString& _family)
{
    const auto row = d->fontFamilyModel->stringList().indexOf(_family);
    d->fontFamily->setCurrentIndex(d->fontFamilyModel->index(row));
}

int ScreenplayTemplateParagraphsView::fontSize() const
{
    return d->fontSize->text().toInt();
}

void ScreenplayTemplateParagraphsView::setFontSize(int _size)
{
    const auto row = d->fontSizeModel->stringList().indexOf(QString::number(_size));
    d->fontSize->setCurrentIndex(d->fontSizeModel->index(row));
}

bool ScreenplayTemplateParagraphsView::isStartsFromNewPage() const
{
    return d->startsFromNewPage->isChecked();
}

void ScreenplayTemplateParagraphsView::setStartsFromNewPage(bool _starts)
{
    d->startsFromNewPage->setChecked(_starts);
}

bool ScreenplayTemplateParagraphsView::isUppercase() const
{
    return d->uppercase->isChecked();
}

void ScreenplayTemplateParagraphsView::setUppercase(bool _uppercase)
{
    d->uppercase->setChecked(_uppercase);
}

bool ScreenplayTemplateParagraphsView::isBold() const
{
    return d->bold->isChecked();
}

void ScreenplayTemplateParagraphsView::setBold(bool _bold)
{
    d->bold->setChecked(_bold);
}

bool ScreenplayTemplateParagraphsView::isItalic() const
{
    return d->italic->isChecked();
}

void ScreenplayTemplateParagraphsView::setItalic(bool _italic)
{
    d->italic->setChecked(_italic);
}

bool ScreenplayTemplateParagraphsView::isUnderline() const
{
    return d->underline->isChecked();
}

void ScreenplayTemplateParagraphsView::setUndeline(bool _underline)
{
    d->underline->setChecked(_underline);
}

Qt::Alignment ScreenplayTemplateParagraphsView::alignment() const
{
    if (d->alignLeft->isChecked()) {
        return Qt::AlignLeft;
    } else if (d->alignCenter->isChecked()) {
        return Qt::AlignHCenter;
    } else if (d->alignRight->isChecked()) {
        return Qt::AlignRight;
    } else {
        return Qt::AlignJustify;
    }
}

void ScreenplayTemplateParagraphsView::setAlignment(Qt::Alignment _alignment)
{
    if (_alignment.testFlag(Qt::AlignLeft)) {
        d->alignLeft->setChecked(true);
    } else if (_alignment.testFlag(Qt::AlignHCenter)) {
        d->alignCenter->setChecked(true);
    } else if (_alignment.testFlag(Qt::AlignRight)) {
        d->alignRight->setChecked(true);
    } else {
        d->alignJustify->setChecked(true);
    }
}

qreal ScreenplayTemplateParagraphsView::topIndent() const
{
    return d->topIndent->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setTopIndent(qreal _indent)
{
    d->topIndent->setText(QString::number(_indent));
}

qreal ScreenplayTemplateParagraphsView::bottomIndent() const
{
    return d->bottomIndent->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setBottomIndent(qreal _indent)
{
    d->bottomIndent->setText(QString::number(_indent));
}

bool ScreenplayTemplateParagraphsView::isVerticalIndentationInLines() const
{
    return d->verticalIndentationInLines->isChecked();
}

void ScreenplayTemplateParagraphsView::setVericalIndentationInLines(bool _inLines)
{
    if (_inLines) {
        d->verticalIndentationInLines->setChecked(true);
    } else {
        d->verticalIndentationInMm->setChecked(true);
    }
}

qreal ScreenplayTemplateParagraphsView::leftIndent() const
{
    return d->leftIndent->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setLeftIndent(qreal _indent)
{
    d->leftIndent->setText(QString::number(_indent));
}

qreal ScreenplayTemplateParagraphsView::rightIndent() const
{
    return d->rightIndent->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setRightIndent(qreal _indent)
{
    d->rightIndent->setText(QString::number(_indent));
}

qreal ScreenplayTemplateParagraphsView::leftIndentInTable() const
{
    return d->leftIndentInTable->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setLeftIndentInTable(qreal _indent)
{
    d->leftIndentInTable->setText(QString::number(_indent));
}

qreal ScreenplayTemplateParagraphsView::rightIndentInTable() const
{
    return d->rightIndentInTable->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setRightIndentInTable(qreal _indent)
{
    d->rightIndentInTable->setText(QString::number(_indent));
}

int ScreenplayTemplateParagraphsView::lineSpacingType() const
{
    return d->lineSpacing->currentIndex().row();
}

void ScreenplayTemplateParagraphsView::setLineSpacingType(int _type)
{
    d->lineSpacing->setCurrentIndex(d->lineSpacingModel->index(_type));
}

qreal ScreenplayTemplateParagraphsView::lineSpacingValue() const
{
    return d->lineSpacingValue->text().toDouble();
}

void ScreenplayTemplateParagraphsView::setLineSpacingValue(qreal _value)
{
    d->lineSpacingValue->setText(QString::number(_value));
}

void ScreenplayTemplateParagraphsView::updateTranslations()
{
    const auto metricSystem = d->useMm ? tr("mm") : tr("inch");
    for (int tab = 0; tab < kParagraphTypes.size(); ++tab) {
        d->paragraphs->setTabName(tab, BusinessLayer::toDisplayString(kParagraphTypes.at(tab)));
    }
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
    d->alignJustify->setText(tr("Justify"));
    d->verticalIndentationTitle->setText(tr("Vertical indentation"));
    d->topIndent->setLabel(tr("Top"));
    d->bottomIndent->setLabel(tr("Bottom"));
    d->verticalIndentationInLines->setText(tr("lines"));
    d->verticalIndentationInMm->setText(metricSystem);
    d->horizontalIndentationTitle->setText(tr("Horizontal indentation"));
    d->leftIndent->setLabel(tr("Left"));
    d->leftIndent->setSuffix(metricSystem);
    d->rightIndent->setLabel(tr("Right"));
    d->rightIndent->setSuffix(metricSystem);
    d->horizontalIndentationInTableTitle->setText(
        tr("Horizontal indentation (for two-column mode)"));
    d->leftIndentInTable->setLabel(tr("Left"));
    d->leftIndentInTable->setSuffix(metricSystem);
    d->rightIndentInTable->setLabel(tr("Right"));
    d->rightIndentInTable->setSuffix(metricSystem);
    d->lineSpacingTitle->setText(tr("Line spacing"));
    d->lineSpacing->setLabel(tr("Type"));
    d->lineSpacingModel->setStringList(
        { tr("Single"), tr("One and half"), tr("Double"), tr("Fixed") });
    d->lineSpacingValue->setLabel(tr("Value"));
    d->lineSpacingValue->setSuffix(metricSystem);
}

void ScreenplayTemplateParagraphsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());
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
             d->alignRight, d->alignJustify, d->verticalIndentationTitle,
             d->verticalIndentationInLines, d->verticalIndentationInMm,
             d->horizontalIndentationTitle, d->horizontalIndentationInTableTitle,
             d->lineSpacingTitle }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().background());
        widget->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto title : { d->textAlignmentTitle, d->verticalIndentationTitle,
                        d->horizontalIndentationTitle, d->lineSpacingTitle }) {
        title->setTextColor(ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                     Ui::DesignSystem::inactiveTextOpacity()));
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
