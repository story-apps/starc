#include "cover_generator_sidebar.h"

#include "unsplash_images_view.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/circular_progress_bar/circular_progress_bar.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/slider/slider.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/tools/debouncer.h>

#include <QBoxLayout>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QScrollArea>
#include <QScrollBar>
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

    /**
     * @brief Страница текстовых параметров
     */
    QScrollArea* textPage = nullptr;
    AbstractLabel* textBackgroundColorTitle = nullptr;
    Slider* textBackgroundColor = nullptr;
    AbstractLabel* textBackgroundColorBlack = nullptr;
    AbstractLabel* textBackgroundColorTransparent = nullptr;
    AbstractLabel* textBackgroundColorWhite = nullptr;
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

    /**
     * @brief Страница параметров изображения
     */
    Widget* imagePage = nullptr;
    TextField* searchImages = nullptr;
    Debouncer searchDebouncer;
    IconButton* pasteImageFromClipboard = nullptr;
    IconButton* chooseImageFile = nullptr;
    UnsplashImagesView* imagesView = nullptr;
    QScrollBar* imagesScrollBar = nullptr;
    Body2Label* imagesLoadingLabel = nullptr;
    CircularProgressBar* imagesLoadingProgress = nullptr;
};

CoverGeneratorSidebar::Implementation::Implementation(QWidget* _parent)
    : tabs(new TabBar(_parent))
    , content(new StackWidget(_parent))
    , textPage(UiHelper::createScrollArea(_parent))
    , textBackgroundColorTitle(new Body1Label(_parent))
    , textBackgroundColor(new Slider(_parent))
    , textBackgroundColorBlack(new Body2Label(_parent))
    , textBackgroundColorTransparent(new Body2Label(_parent))
    , textBackgroundColorWhite(new Body2Label(_parent))
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
    , searchImages(new TextField(_parent))
    , searchDebouncer(300)
    , pasteImageFromClipboard(new IconButton(_parent))
    , chooseImageFile(new IconButton(_parent))
    , imagesView(new UnsplashImagesView(_parent))
    , imagesLoadingLabel(new Body2Label(_parent))
    , imagesLoadingProgress(new CircularProgressBar(_parent))
{
    fontsModel.setStringList(QFontDatabase().families());

    tabs->addTab({});
    tabs->addTab({});

    content->setCurrentWidget(textPage);
    content->addWidget(imagePage);

    textBackgroundColor->setMaximumValue(200);
    textBackgroundColor->setDefaultValue(100);
    textBackgroundColor->setValue(100);
    textBackgroundColorTransparent->setAlignment(Qt::AlignCenter);
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

    pasteImageFromClipboard->setIcon(u8"\U000F0192");
    chooseImageFile->setIcon(u8"\U000F0552");

    auto imagesScrollArea = UiHelper::createScrollArea(_parent);
    imagesScrollArea->setWidget(imagesView);
    imagesScrollBar = imagesScrollArea->verticalScrollBar();
    imagesLoadingLabel->hide();
    imagesLoadingProgress->hide();


    auto textLayout = qobject_cast<QVBoxLayout*>(textPage->widget()->layout());
    auto stretch = textLayout->takeAt(0);
    textLayout->addWidget(textBackgroundColorTitle);
    textLayout->addWidget(textBackgroundColor);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(textBackgroundColorBlack);
        layout->addWidget(textBackgroundColorTransparent, 1);
        layout->addWidget(textBackgroundColorWhite);
        textLayout->addLayout(layout);
    }
    textLayout->addWidget(top1Text);
    textLayout->addWidget(top2Text);
    textLayout->addWidget(beforeNameText);
    textLayout->addWidget(nameText);
    textLayout->addWidget(afterNameText);
    textLayout->addWidget(creditsText);
    textLayout->addWidget(releaseDateText);
    textLayout->addWidget(websiteText);
    textLayout->addItem(stretch);

    auto imageSourceLayout = new QHBoxLayout;
    imageSourceLayout->setContentsMargins({});
    imageSourceLayout->setSpacing(0);
    imageSourceLayout->addWidget(searchImages, 1);
    imageSourceLayout->addWidget(pasteImageFromClipboard, 0, Qt::AlignVCenter);
    imageSourceLayout->addWidget(chooseImageFile, 0, Qt::AlignVCenter);
    auto imageLayout = new QVBoxLayout;
    imageLayout->setContentsMargins({});
    imageLayout->setSpacing(0);
    imageLayout->addLayout(imageSourceLayout);
    imageLayout->addWidget(imagesScrollArea, 1);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(imagesLoadingLabel, 1, Qt::AlignVCenter);
        layout->addWidget(imagesLoadingProgress);
        imageLayout->addLayout(layout);
    }
    imagePage->setLayout(imageLayout);
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
    //
    connect(d->textBackgroundColor, &Slider::valueChanged, this,
            &CoverGeneratorSidebar::textBackgroundColorChanged);
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
                &CoverGeneratorSidebar::textParametersChanged);
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
                &CoverGeneratorSidebar::textParametersChanged);
        connect(textFieldWithOptions->fontSize, &TextField::textChanged, this,
                &CoverGeneratorSidebar::textParametersChanged);
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

                            emit textParametersChanged();
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
                    &CoverGeneratorSidebar::textParametersChanged);
        }
    }
    //
    connect(d->searchImages, &TextField::textChanged, &d->searchDebouncer, &Debouncer::orderWork);
    connect(&d->searchDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->searchImages->text().isEmpty()) {
            return;
        }

        //
        // Формируем английский перевод поисковой фразы
        //
        const QUrl keywordsTranslateUrl(QString("https://translate.googleapis.com/translate_a/"
                                                "t?client=dict-chrome-ex&sl=auto&tl=en&q=%1")
                                            .arg(d->searchImages->text()));
        NetworkRequestLoader::loadAsync(
            keywordsTranslateUrl, this, [this](const QByteArray& _response) {
                const auto translations = QJsonDocument::fromJson(_response).array();
                if (translations.isEmpty() || translations.at(0).toArray().isEmpty()) {
                    return;
                }
                const auto keywords = translations.at(0).toArray().at(0).toString();

                //
                // Запрашиваем картинки
                //
                d->imagesView->loadImages(keywords);
            });
    });
    connect(d->imagesScrollBar, &QScrollBar::valueChanged, this, [this](int _value) {
        if (_value == d->imagesScrollBar->maximum()) {
            d->imagesView->loadNextImagesPage();
        }
    });
    connect(d->imagesView, &UnsplashImagesView::imagesLoadingProgressChanged, this,
            [this](qreal _progress) {
                d->imagesLoadingProgress->setProgress(_progress);
                d->imagesLoadingLabel->show();
                d->imagesLoadingProgress->show();
            });
    connect(d->imagesView, &UnsplashImagesView::imagesLoaded, this, [this] {
        d->imagesScrollBar->setValue(d->imagesScrollBar->value()
                                     + d->imagesLoadingProgress->sizeHint().height());
        d->imagesLoadingLabel->hide();
        d->imagesLoadingProgress->hide();
    });
    connect(d->imagesView, &UnsplashImagesView::imageSelected, this,
            &CoverGeneratorSidebar::unsplashImageSelected);
    connect(d->pasteImageFromClipboard, &IconButton::clicked, this,
            &CoverGeneratorSidebar::pasteImageFromClipboardPressed);
    connect(d->chooseImageFile, &IconButton::clicked, this,
            &CoverGeneratorSidebar::chooseImgeFromFilePressed);
}

CoverGeneratorSidebar::~CoverGeneratorSidebar() = default;

void CoverGeneratorSidebar::clear()
{
    d->textBackgroundColor->setValue(100);
    d->top1Text->text->clear();
    d->top1Text->fontFamily->setCurrentText("Montserrat");
    d->top1Text->fontSize->setText("16");
    d->top1Text->alignCenter->setChecked(true);
    d->top2Text->text->clear();
    d->top2Text->fontFamily->setCurrentText("Montserrat");
    d->top2Text->fontSize->setText("16");
    d->top2Text->fontBold->setChecked(true);
    d->top2Text->alignCenter->setChecked(true);
    d->beforeNameText->text->clear();
    d->beforeNameText->fontFamily->setCurrentText("Montserrat");
    d->beforeNameText->fontSize->setText("22");
    d->beforeNameText->alignCenter->setChecked(true);
    d->nameText->text->clear();
    d->nameText->fontFamily->setCurrentText("Montserrat");
    d->nameText->fontSize->setText("44");
    d->nameText->fontBold->setChecked(true);
    d->nameText->alignCenter->setChecked(true);
    d->afterNameText->text->clear();
    d->afterNameText->fontFamily->setCurrentText("Montserrat");
    d->afterNameText->fontSize->setText("18");
    d->afterNameText->alignCenter->setChecked(true);
    d->creditsText->text->clear();
    d->creditsText->fontFamily->setCurrentText("SF Movie Poster");
    d->creditsText->fontSize->setText("24");
    d->creditsText->alignCenter->setChecked(true);
    d->releaseDateText->text->clear();
    d->releaseDateText->fontFamily->setCurrentText("Montserrat");
    d->releaseDateText->fontSize->setText("16");
    d->releaseDateText->fontBold->setChecked(true);
    d->releaseDateText->alignCenter->setChecked(true);
    d->websiteText->text->clear();
    d->websiteText->fontFamily->setCurrentText("Montserrat");
    d->websiteText->fontSize->setText("12");
    d->websiteText->fontBold->setChecked(true);
    d->websiteText->alignCenter->setChecked(true);
}

QColor CoverGeneratorSidebar::textBackgroundColor() const
{
    if (d->textBackgroundColor->value() < 100) {
        QColor color = Qt::black;
        color.setAlphaF((100.0 - d->textBackgroundColor->value()) / 100.0);
        return color;
    } else if (d->textBackgroundColor->value() == 100) {
        return {};
    } else { // > 100
        QColor color = Qt::white;
        color.setAlphaF((d->textBackgroundColor->value() - 100.0) / 100.0);
        return color;
    }
}

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

void CoverGeneratorSidebar::updateTranslations()
{
    d->tabs->setTabName(kTextIndex, tr("Text"));
    d->tabs->setTabName(kImageIndex, tr("Background image"));
    d->textBackgroundColorTitle->setText(tr("Color between text and image"));
    d->textBackgroundColorBlack->setText(tr("black"));
    d->textBackgroundColorTransparent->setText(tr("transparent"));
    d->textBackgroundColorWhite->setText(tr("white"));
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

    d->searchImages->setLabel(tr("Search images"));
    d->searchImages->setHelper(tr("Enter keywords for search"));
    d->pasteImageFromClipboard->setToolTip(
        tr("Paste image from clipboard (image file or image url)"));
    d->chooseImageFile->setToolTip(tr("Choose file with image"));
    d->imagesLoadingLabel->setText(tr("Loading images"));
}

void CoverGeneratorSidebar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->tabs->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->tabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->content->setBackgroundColor(Ui::DesignSystem::color().primary());

    d->textPage->widget()->layout()->setContentsMargins(
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12());
    for (auto label : {
             d->textBackgroundColorTitle,
             d->textBackgroundColorBlack,
             d->textBackgroundColorTransparent,
             d->textBackgroundColorWhite,
         }) {
        label->setBackgroundColor(DesignSystem::color().primary());
        label->setTextColor(DesignSystem::color().onPrimary());
    }
    d->textBackgroundColorTitle->setContentsMargins(DesignSystem::layout().px16(),
                                                    DesignSystem::layout().px12(),
                                                    DesignSystem::layout().px16(), 0);
    d->textBackgroundColorBlack->setContentsMargins(DesignSystem::layout().px16(), 0,
                                                    DesignSystem::layout().px16(),
                                                    DesignSystem::layout().px8());
    d->textBackgroundColorTransparent->setContentsMargins(0, 0, 0, DesignSystem::layout().px8());
    d->textBackgroundColorWhite->setContentsMargins(DesignSystem::layout().px16(), 0,
                                                    DesignSystem::layout().px16(),
                                                    DesignSystem::layout().px8());
    d->textBackgroundColor->setBackgroundColor(DesignSystem::color().primary());
    d->textBackgroundColor->setTextColor(DesignSystem::color().onPrimary());
    d->textBackgroundColor->setContentsMargins(DesignSystem::layout().px16(), 0,
                                               DesignSystem::layout().px16(), 0);
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
            { isLeftToRight() ? Ui::DesignSystem::layout().px12() : 0.0, 0.0,
              isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px12(), 0.0 });
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
        textFieldWithOptions->fontColor->setContentsMargins(
            isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px12(), 0,
            isLeftToRight() ? Ui::DesignSystem::layout().px12() : 0.0, 0);
        textFieldWithOptions->fontBold->setContentsMargins(
            isLeftToRight() ? Ui::DesignSystem::layout().px12() : 0.0,
            Ui::DesignSystem::layout().px12(),
            isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px12(),
            Ui::DesignSystem::layout().px12());
        textFieldWithOptions->alignLeft->setContentsMargins(
            isLeftToRight() ? Ui::DesignSystem::layout().px24() : 0.0,
            Ui::DesignSystem::layout().px12(),
            isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px24(),
            Ui::DesignSystem::layout().px12());
    }
    d->colorPickerPopup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->colorPickerPopup->setTextColor(Ui::DesignSystem::color().onPrimary());


    d->imagePage->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->searchImages->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->searchImages->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->searchImages->setCustomMargins(
        { isLeftToRight() ? Ui::DesignSystem::layout().px24() : Ui::DesignSystem::layout().px12(),
          Ui::DesignSystem::layout().px24(),
          isLeftToRight() ? Ui::DesignSystem::layout().px12() : Ui::DesignSystem::layout().px24(),
          Ui::DesignSystem::layout().px24() });
    for (auto button : {
             d->pasteImageFromClipboard,
             d->chooseImageFile,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().primary());
        button->setTextColor(Ui::DesignSystem::color().onPrimary());
    }
    d->chooseImageFile->setContentsMargins(
        isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px12(), 0,
        isLeftToRight() ? Ui::DesignSystem::layout().px12() : 0.0, 0);
    d->imagesView->setBackgroundColor(Ui::DesignSystem::color().primary());
    for (auto widget : std::vector<Widget*>{
             d->imagesLoadingLabel,
             d->imagesLoadingProgress,
         }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().primary());
        widget->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                      Ui::DesignSystem::inactiveTextOpacity()));
    }
    d->imagesLoadingLabel->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                              Ui::DesignSystem::layout().px24(), 0);
    d->imagesLoadingProgress->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12());
}

} // namespace Ui
