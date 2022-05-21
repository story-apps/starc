#include "cover_generator_sidebar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QFontDatabase>
#include <QScrollArea>
#include <QStringListModel>


namespace Ui {

namespace {
constexpr int kTextIndex = 0;
constexpr int kImageIndex = 1;


//
// TODO: отнаследоваться от ResizableWidget, но сперва его докрутить, чтобы работал красиво
//
class TextFiledWithOptions : public Widget
{
public:
    explicit TextFiledWithOptions(QWidget* _parent = nullptr);

    /**
     * @brief Настроить видимость опций текста
     */
    void setOptionsVisible(bool _visible);

    /**
     * @brief Получить сформированный объект с параметрами редактора
     */
    CoverTextParameters parameters() const;


    bool isColorSet = false;

    TextField* text = nullptr;
    ComboBox* fontFamily = nullptr;
    TextField* fontSize = nullptr;
    IconButton* fontColor = nullptr;
    IconButton* fontBold = nullptr;
    IconButton* fontItalic = nullptr;
    IconButton* fontUnderline = nullptr;
    IconButton* alignLeft = nullptr;
    IconButton* alignCenter = nullptr;
    IconButton* alignRight = nullptr;
};

TextFiledWithOptions::TextFiledWithOptions(QWidget* _parent)
    : Widget(_parent)
    , text(new TextField(_parent))
    , fontFamily(new ComboBox(_parent))
    , fontSize(new TextField(_parent))
    , fontColor(new IconButton(_parent))
    , fontBold(new IconButton(_parent))
    , fontItalic(new IconButton(_parent))
    , fontUnderline(new IconButton(_parent))
    , alignLeft(new IconButton(_parent))
    , alignCenter(new IconButton(_parent))
    , alignRight(new IconButton(_parent))
{
    text->setTrailingIcon(u8"\U000F0493");
    fontColor->setIcon(u8"\U000F0766");
    fontBold->setIcon(u8"\U000F0264");
    fontBold->setCheckable(true);
    fontItalic->setIcon(u8"\U000F0277");
    fontItalic->setCheckable(true);
    fontUnderline->setIcon(u8"\U000F0287");
    fontUnderline->setCheckable(true);
    alignLeft->setIcon(u8"\U000F0262");
    alignLeft->setCheckable(true);
    alignCenter->setIcon(u8"\U000F0260");
    alignCenter->setCheckable(true);
    alignRight->setIcon(u8"\U000F0263");
    alignRight->setCheckable(true);

    setOptionsVisible(false);

    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(fontFamily, 2);
    topLayout->addWidget(fontSize, 1);
    topLayout->addWidget(fontColor);

    auto bottomLayout = new QHBoxLayout;
    bottomLayout->setContentsMargins({});
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(fontBold);
    bottomLayout->addWidget(fontItalic);
    bottomLayout->addWidget(fontUnderline);
    bottomLayout->addWidget(alignLeft);
    bottomLayout->addWidget(alignCenter);
    bottomLayout->addWidget(alignRight);
    bottomLayout->addStretch();

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(text);
    layout->addLayout(topLayout);
    layout->addLayout(bottomLayout);
    setLayout(layout);


    connect(alignLeft, &IconButton::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            alignCenter->setChecked(false);
            alignRight->setChecked(false);
        }
    });
    connect(alignCenter, &IconButton::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            alignLeft->setChecked(false);
            alignRight->setChecked(false);
        }
    });
    connect(alignRight, &IconButton::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            alignLeft->setChecked(false);
            alignCenter->setChecked(false);
        }
    });
}

void TextFiledWithOptions::setOptionsVisible(bool _visible)
{
    for (auto widget : std::vector<QWidget*>{
             fontFamily,
             fontSize,
             fontColor,
             fontBold,
             fontItalic,
             fontUnderline,
             alignLeft,
             alignCenter,
             alignRight,
         }) {
        widget->setVisible(_visible);
    }
}

CoverTextParameters TextFiledWithOptions::parameters() const
{
    QFont font(fontFamily->text(), fontSize->text().toInt());
    font.setBold(fontBold->isChecked());
    font.setItalic(fontItalic->isChecked());
    font.setUnderline(fontUnderline->isChecked());
    const auto color
        = isColorSet ? fontColor->textColor() : Ui::DesignSystem::color().onBackground();
    const auto align = alignLeft->isChecked()
        ? Qt::AlignLeft
        : (alignCenter->isChecked() ? Qt::AlignHCenter : Qt::AlignRight);
    return { text->text(), font, color, align };
}

} // namespace

class CoverGeneratorSidebar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QStringListModel fontsModel;

    TabBar* tabs = nullptr;
    StackWidget* content = nullptr;

    QScrollArea* textPage = nullptr;
    TextFiledWithOptions* top1Text = nullptr;
    TextFiledWithOptions* top2Text = nullptr;
    TextFiledWithOptions* beforeNameText = nullptr;
    TextFiledWithOptions* nameText = nullptr;
    TextFiledWithOptions* afterNameText = nullptr;
    TextFiledWithOptions* creditsText = nullptr;
    TextFiledWithOptions* releaseDateText = nullptr;
    TextFiledWithOptions* websiteText = nullptr;

    /**
     * @brief Попап выбора цвета
     */
    ColorPickerPopup* colorPickerPopup = nullptr;

    Widget* imagePage = nullptr;
};

CoverGeneratorSidebar::Implementation::Implementation(QWidget* _parent)
    : tabs(new TabBar(_parent))
    , content(new StackWidget(_parent))
    , textPage(UiHelper::createScrollArea(_parent))
    , top1Text(new TextFiledWithOptions(_parent))
    , top2Text(new TextFiledWithOptions(_parent))
    , beforeNameText(new TextFiledWithOptions(_parent))
    , nameText(new TextFiledWithOptions(_parent))
    , afterNameText(new TextFiledWithOptions(_parent))
    , creditsText(new TextFiledWithOptions(_parent))
    , releaseDateText(new TextFiledWithOptions(_parent))
    , websiteText(new TextFiledWithOptions(_parent))
    , colorPickerPopup(new ColorPickerPopup(_parent))
    , imagePage(new Widget(_parent))
{
    fontsModel.setStringList(QFontDatabase().families());

    tabs->addTab({});
    tabs->addTab({});

    content->setCurrentWidget(textPage);
    content->addWidget(imagePage);

    for (auto textFieldWithOptions : {
             top1Text,
             top2Text,
             beforeNameText,
             nameText,
             afterNameText,
             creditsText,
             releaseDateText,
             websiteText,
         }) {
        textFieldWithOptions->fontFamily->setModel(&fontsModel);
    }
    top1Text->fontFamily->setCurrentText("Montserrat");
    top1Text->fontSize->setText("16");
    top1Text->alignCenter->setChecked(true);
    top2Text->fontFamily->setCurrentText("Montserrat");
    top2Text->fontSize->setText("16");
    top2Text->fontBold->setChecked(true);
    top2Text->alignCenter->setChecked(true);
    beforeNameText->fontFamily->setCurrentText("Montserrat");
    beforeNameText->fontSize->setText("22");
    beforeNameText->alignCenter->setChecked(true);
    nameText->fontFamily->setCurrentText("Montserrat");
    nameText->fontSize->setText("44");
    nameText->fontBold->setChecked(true);
    nameText->alignCenter->setChecked(true);
    afterNameText->fontFamily->setCurrentText("Montserrat");
    afterNameText->fontSize->setText("18");
    afterNameText->alignCenter->setChecked(true);
    creditsText->fontFamily->setCurrentText("SF Movie Poster");
    creditsText->fontSize->setText("24");
    creditsText->alignCenter->setChecked(true);
    releaseDateText->fontFamily->setCurrentText("Montserrat");
    releaseDateText->fontSize->setText("16");
    releaseDateText->fontBold->setChecked(true);
    releaseDateText->alignCenter->setChecked(true);
    websiteText->fontFamily->setCurrentText("Montserrat");
    websiteText->fontSize->setText("12");
    websiteText->fontBold->setChecked(true);
    websiteText->alignCenter->setChecked(true);

    colorPickerPopup->setColorCanBeDeselected(true);


    auto textLayout = qobject_cast<QVBoxLayout*>(textPage->widget()->layout());
    auto stretch = textLayout->takeAt(0);
    textLayout->addWidget(top1Text);
    textLayout->addWidget(top2Text);
    textLayout->addWidget(beforeNameText);
    textLayout->addWidget(nameText);
    textLayout->addWidget(afterNameText);
    textLayout->addWidget(creditsText);
    textLayout->addWidget(releaseDateText);
    textLayout->addWidget(websiteText);
    textLayout->addItem(stretch);
}


// ****


CoverGeneratorSidebar::CoverGeneratorSidebar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tabs);
    layout->addWidget(d->content, 1);
    setLayout(layout);

    connect(d->tabs, &TabBar::currentIndexChanged, this, [this](int _index) {
        switch (_index) {
        default:
        case kTextIndex: {
            d->content->setCurrentWidget(d->textPage);
            break;
        }

        case kImageIndex: {
            d->content->setCurrentWidget(d->imagePage);
            break;
        }
        }
    });
    for (auto textFieldWithOptions : {
             d->top1Text,
             d->top2Text,
             d->beforeNameText,
             d->nameText,
             d->afterNameText,
             d->creditsText,
             d->releaseDateText,
             d->websiteText,
         }) {
        connect(textFieldWithOptions->text, &TextField::textChanged, this,
                &CoverGeneratorSidebar::coverParametersChanged);
        connect(textFieldWithOptions->text, &TextField::trailingIconPressed, this,
                [textFieldWithOptions] {
                    const auto isOptionsVisible = textFieldWithOptions->fontFamily->isVisible();
                    textFieldWithOptions->setBackgroundColor(
                        isOptionsVisible
                            ? Ui::DesignSystem::color().primary()
                            : ColorHelper::nearby(Ui::DesignSystem::color().primary()));
                    textFieldWithOptions->text->setTrailingIconColor(
                        isOptionsVisible ? QColor() : Ui::DesignSystem::color().secondary());
                    textFieldWithOptions->setOptionsVisible(!isOptionsVisible);
                });
        connect(textFieldWithOptions->fontFamily, &ComboBox::currentIndexChanged, this,
                &CoverGeneratorSidebar::coverParametersChanged);
        connect(textFieldWithOptions->fontSize, &TextField::textChanged, this,
                &CoverGeneratorSidebar::coverParametersChanged);
        connect(
            textFieldWithOptions->fontColor, &IconButton::clicked, this,
            [this, textFieldWithOptions] {
                d->colorPickerPopup->setSelectedColor(
                    textFieldWithOptions->isColorSet ? textFieldWithOptions->fontColor->textColor()
                                                     : QColor());
                d->colorPickerPopup->showPopup(textFieldWithOptions->fontColor,
                                               Qt::AlignBottom | Qt::AlignRight);
                d->colorPickerPopup->disconnect(this);
                connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged, this,
                        [this, textFieldWithOptions](const QColor& _color) {
                            textFieldWithOptions->isColorSet = _color.isValid();
                            textFieldWithOptions->fontColor->setIcon(
                                textFieldWithOptions->isColorSet ? u8"\U000F0765" : u8"\U000f0766");
                            textFieldWithOptions->fontColor->setTextColor(
                                textFieldWithOptions->isColorSet
                                    ? _color
                                    : Ui::DesignSystem::color().onPrimary());

                            emit coverParametersChanged();
                        });
            });
        for (auto iconButton : {
                 textFieldWithOptions->fontBold,
                 textFieldWithOptions->fontItalic,
                 textFieldWithOptions->fontUnderline,
                 textFieldWithOptions->alignLeft,
                 textFieldWithOptions->alignCenter,
                 textFieldWithOptions->alignRight,
             }) {
            connect(iconButton, &IconButton::checkedChanged, this,
                    &CoverGeneratorSidebar::coverParametersChanged);
        }
    }
}

CoverGeneratorSidebar::~CoverGeneratorSidebar() = default;

CoverTextParameters CoverGeneratorSidebar::top1Text() const
{
    return d->top1Text->parameters();
}

CoverTextParameters CoverGeneratorSidebar::top2Text() const
{
    return d->top2Text->parameters();
}

CoverTextParameters CoverGeneratorSidebar::beforeNameText() const
{
    return d->beforeNameText->parameters();
}

CoverTextParameters CoverGeneratorSidebar::nameText() const
{
    return d->nameText->parameters();
}

CoverTextParameters CoverGeneratorSidebar::afterNameText() const
{
    return d->afterNameText->parameters();
}

CoverTextParameters CoverGeneratorSidebar::creditsText() const
{
    return d->creditsText->parameters();
}

CoverTextParameters CoverGeneratorSidebar::releaseDateText() const
{
    return d->releaseDateText->parameters();
}

CoverTextParameters CoverGeneratorSidebar::websiteText() const
{
    return d->websiteText->parameters();
}

QPixmap CoverGeneratorSidebar::backgroundImage() const
{
    return {};
}

void CoverGeneratorSidebar::updateTranslations()
{
    d->tabs->setTabName(kTextIndex, tr("Text"));
    d->tabs->setTabName(kImageIndex, tr("Background image"));
    d->top1Text->text->setLabel(tr("Text of the first top line"));
    d->top2Text->text->setLabel(tr("Text of the second top line"));
    d->beforeNameText->text->setLabel(tr("Text above of the project name"));
    d->nameText->text->setLabel(tr("Project name"));
    d->afterNameText->text->setLabel(tr("Text under the project name"));
    d->creditsText->text->setLabel(tr("Credits"));
    d->releaseDateText->text->setLabel(tr("Release date or coming soon"));
    d->websiteText->text->setLabel(tr("Website"));
    for (auto textField : {
             d->top1Text,
             d->top2Text,
             d->beforeNameText,
             d->nameText,
             d->afterNameText,
             d->creditsText,
             d->releaseDateText,
             d->websiteText,
         }) {
        textField->fontFamily->setLabel(tr("Font family"));
        textField->fontSize->setLabel(tr("Size"));
        textField->fontColor->setToolTip(tr("Text color"));
        textField->fontBold->setToolTip(tr("Make text bold"));
        textField->fontItalic->setToolTip(tr("Make text italic"));
        textField->fontUnderline->setToolTip(tr("Make text underlined"));
        textField->alignLeft->setToolTip(tr("Align text to the left"));
        textField->alignCenter->setToolTip(tr("Align text to the center"));
        textField->alignRight->setToolTip(tr("Align text to the right"));
    }
}

void CoverGeneratorSidebar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->tabs->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->tabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->content->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->imagePage->setBackgroundColor(Ui::DesignSystem::color().primary());
    for (auto textFieldWithOptions : {
             d->top1Text,
             d->top2Text,
             d->beforeNameText,
             d->nameText,
             d->afterNameText,
             d->creditsText,
             d->releaseDateText,
             d->websiteText,
         }) {
        textFieldWithOptions->setBackgroundColor(Ui::DesignSystem::color().primary());
        for (auto textField : std::vector<TextField*>{
                 textFieldWithOptions->text,
                 textFieldWithOptions->fontFamily,
                 textFieldWithOptions->fontSize,
             }) {
            textField->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
            textField->setTextColor(Ui::DesignSystem::color().onPrimary());
        }
        textFieldWithOptions->text->setCustomMargins(
            { Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
              Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12() });
        textFieldWithOptions->fontFamily->setCustomMargins(
            { Ui::DesignSystem::layout().px12(), 0.0, 0.0, 0.0 });
        textFieldWithOptions->fontFamily->setPopupBackgroundColor(
            ColorHelper::nearby(Ui::DesignSystem::color().primary()));
        textFieldWithOptions->fontSize->setCustomMargins(
            { Ui::DesignSystem::layout().px12(), 0.0, Ui::DesignSystem::layout().px12(), 0.0 });
        for (auto iconButton : {
                 textFieldWithOptions->fontColor,
                 textFieldWithOptions->fontBold,
                 textFieldWithOptions->fontItalic,
                 textFieldWithOptions->fontUnderline,
                 textFieldWithOptions->alignLeft,
                 textFieldWithOptions->alignCenter,
                 textFieldWithOptions->alignRight,
             }) {
            iconButton->setBackgroundColor(
                ColorHelper::nearby(Ui::DesignSystem::color().primary()));
            iconButton->setTextColor(Ui::DesignSystem::color().onPrimary());
            iconButton->setContentsMargins(0, Ui::DesignSystem::layout().px12(), 0,
                                           Ui::DesignSystem::layout().px12());
        }
        textFieldWithOptions->fontColor->setTextColor(Ui::DesignSystem::color().onPrimary());
        textFieldWithOptions->fontColor->setContentsMargins(0, 0, Ui::DesignSystem::layout().px12(),
                                                            0);
        textFieldWithOptions->fontBold->setContentsMargins(Ui::DesignSystem::layout().px12(),
                                                           Ui::DesignSystem::layout().px12(), 0,
                                                           Ui::DesignSystem::layout().px12());
        textFieldWithOptions->alignLeft->setContentsMargins(Ui::DesignSystem::layout().px24(),
                                                            Ui::DesignSystem::layout().px12(), 0,
                                                            Ui::DesignSystem::layout().px12());
    }
    d->colorPickerPopup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->colorPickerPopup->setTextColor(Ui::DesignSystem::color().onPrimary());

    d->textPage->widget()->layout()->setContentsMargins(
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12());
}

} // namespace Ui
