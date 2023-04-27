#include "text_generation_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>


namespace Ui {

namespace {

/**
 * @brief Класс странички с конкретной функцией ИИ помощника
 */
class Page : public Widget
{
public:
    explicit Page(QWidget* _parent = nullptr)
        : Widget(_parent)
        , backButton(new IconButton(_parent))
        , titleLabel(new Subtitle2Label(_parent))
        , contentsLayout(new QVBoxLayout)
    {
        backButton->setIcon(u8"\U000F0141");

        contentsLayout->setContentsMargins({});
        contentsLayout->setSpacing(0);
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(backButton);
        titleLayout->addWidget(titleLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
        contentsLayout->addLayout(titleLayout);

        setLayout(contentsLayout);
    }

    IconButton* backButton = nullptr;
    Subtitle2Label* titleLabel = nullptr;
    QVBoxLayout* contentsLayout = nullptr;
};

} // namespace

class TextGenerationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    bool isReadOnly = false;

    StackWidget* pages = nullptr;

    Widget* buttonsPage = nullptr;
    Button* openRephraseButton = nullptr;
    Button* openTranslteButton = nullptr;
    Button* openInsertButton = nullptr;
    Button* openExpandButton = nullptr;
    Button* openShortenButton = nullptr;
    Button* openGenerateButton = nullptr;
    Button* openSummarizeButton = nullptr;

    Page* rephrasePage = nullptr;
    TextField* rephraseSourceText = nullptr;
    TextField* rephraseStyleText = nullptr;
    TextField* rephraseResultText = nullptr;
    Button* rephraseInsertButton = nullptr;
    Button* rephraseButton = nullptr;
    QHBoxLayout* rephraseButtonsLayout = nullptr;


    CaptionLabel* availableWordsLabel = nullptr;
    Button* buyCreditsButton = nullptr;
};

TextGenerationView::Implementation::Implementation(QWidget* _parent)
    : pages(new StackWidget(_parent))
    , buttonsPage(new Widget(pages))
    , openRephraseButton(new Button(buttonsPage))
    , openTranslteButton(new Button(buttonsPage))
    , openInsertButton(new Button(buttonsPage))
    , openExpandButton(new Button(buttonsPage))
    , openShortenButton(new Button(buttonsPage))
    , openGenerateButton(new Button(buttonsPage))
    , openSummarizeButton(new Button(buttonsPage))
    //
    , rephrasePage(new Page(pages))
    , rephraseSourceText(new TextField(rephrasePage))
    , rephraseStyleText(new TextField(rephrasePage))
    , rephraseResultText(new TextField(rephrasePage))
    , rephraseInsertButton(new Button(rephrasePage))
    , rephraseButton(new Button(rephrasePage))
    , rephraseButtonsLayout(new QHBoxLayout)
    //
    , availableWordsLabel(new CaptionLabel(_parent))
    , buyCreditsButton(new Button(_parent))
{
    pages->setAnimationType(StackWidget::AnimationType::Slide);
    pages->setCurrentWidget(buttonsPage);
    pages->addWidget(rephrasePage);

    {
        openRephraseButton->setIcon(u8"\U000F0456");
        openTranslteButton->setIcon(u8"\U000F05CA");
        openInsertButton->setIcon(u8"\U000F0B33");
        openExpandButton->setIcon(u8"\U000F084F");
        openShortenButton->setIcon(u8"\U000F084D");
        openGenerateButton->setIcon(u8"\U000F027F");
        openSummarizeButton->setIcon(u8"\U000F0DD0");
        for (auto button : {
                 openRephraseButton,
                 openTranslteButton,
                 openInsertButton,
                 openExpandButton,
                 openShortenButton,
                 openGenerateButton,
                 openSummarizeButton,
             }) {
            button->setFlat(true);
        }

        auto layout = new QVBoxLayout(buttonsPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(openRephraseButton);
        layout->addWidget(openTranslteButton);
        layout->addWidget(openInsertButton);
        layout->addWidget(openExpandButton);
        layout->addWidget(openShortenButton);
        layout->addWidget(openGenerateButton);
        layout->addWidget(openSummarizeButton);
        layout->addStretch();
    }

    {
        rephraseResultText->hide();
        rephraseInsertButton->hide();
        rephraseButtonsLayout->setContentsMargins({});
        rephraseButtonsLayout->setSpacing(0);
        rephraseButtonsLayout->addStretch();
        rephraseButtonsLayout->addWidget(rephraseInsertButton);
        rephraseButtonsLayout->addWidget(rephraseButton);


        auto layout = rephrasePage->contentsLayout;
        layout->addWidget(rephraseSourceText);
        layout->addWidget(rephraseStyleText);
        layout->addWidget(rephraseResultText);
        layout->addLayout(rephraseButtonsLayout);
        layout->addStretch();
    }
}


// ****


TextGenerationView::TextGenerationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->pages, 1);
    layout->addWidget(d->availableWordsLabel);
    layout->addWidget(d->buyCreditsButton);

    connect(d->openRephraseButton, &Button::clicked, this, [this] {
        d->rephraseResultText->hide();
        d->rephraseInsertButton->hide();
        d->pages->setCurrentWidget(d->rephrasePage);
    });
    for (auto page : {
             d->rephrasePage,
         }) {
        connect(page->backButton, &IconButton::clicked, this,
                [this] { d->pages->setCurrentWidget(d->buttonsPage); });
        connect(page->titleLabel, &Subtitle2Label::clicked, this,
                [this] { d->pages->setCurrentWidget(d->buttonsPage); });
    }
    connect(d->rephraseButton, &Button::clicked, this, [this] {
        emit rephraseRequested(d->rephraseSourceText->text(), d->rephraseStyleText->text());
    });
    connect(d->rephraseInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->rephraseResultText->text()); });
}

TextGenerationView::~TextGenerationView()
{
}

bool TextGenerationView::isReadOnly() const
{
    return d->isReadOnly;
}

void TextGenerationView::setReadOnly(bool _readOnly)
{
    if (d->isReadOnly == _readOnly) {
        return;
    }

    d->isReadOnly = _readOnly;
    for (auto button : {
             d->openRephraseButton,
             d->openTranslteButton,
             d->openInsertButton,
             d->openExpandButton,
             d->openShortenButton,
             d->openGenerateButton,
             d->openSummarizeButton,
         }) {
        button->setEnabled(!_readOnly);
    }

    //
    // TODO: все остальные поля тоже задизейблить
    //
}

void TextGenerationView::setRephraseResult(const QString& _text)
{
    d->rephraseResultText->setText(_text);
    d->rephraseResultText->show();
    d->rephraseInsertButton->show();
}

void TextGenerationView::setAvailableWords(int _availableWords)
{
    d->availableWordsLabel->setText(_availableWords > 0
                                        ? tr("%n word(s) available", nullptr, _availableWords)
                                        : tr("No words available"));

    //
    // TODO: Заблокировать кнопки генерации, если кончились кредиты и показать кнопку пополнения
    //
}

void TextGenerationView::updateTranslations()
{
    d->openRephraseButton->setText(tr("Rephrase"));
    d->openTranslteButton->setText(tr("Translate"));
    d->openInsertButton->setText(tr("Insert"));
    d->openExpandButton->setText(tr("Expand"));
    d->openShortenButton->setText(tr("Shorten"));
    d->openGenerateButton->setText(tr("Generate"));
    d->openSummarizeButton->setText(tr("Summarize"));

    for (auto page : {
             d->rephrasePage,
         }) {
        page->backButton->setToolTip(tr("Go back to list of assistant functions"));
    }
    d->rephrasePage->titleLabel->setText(tr("Rephrase"));
    d->rephraseSourceText->setLabel(tr("Text to rephrase"));
    d->rephraseStyleText->setLabel(tr("Rephrasing style"));
    d->rephraseStyleText->setHelper(tr("Keep empty to avoid style changing"));
    d->rephraseResultText->setLabel(tr("Rephrased text"));
    d->rephraseButton->setText(tr("Rephrase"));
    d->rephraseInsertButton->setText(tr("Insert"));

    d->buyCreditsButton->setText(tr("Buy credits"));
}

void TextGenerationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    d->pages->setBackgroundColor(DesignSystem::color().primary());

    d->buttonsPage->setBackgroundColor(DesignSystem::color().primary());
    d->buttonsPage->setTextColor(DesignSystem::color().onPrimary());
    d->buttonsPage->layout()->setSpacing(DesignSystem::compactLayout().px16());
    d->buttonsPage->layout()->setContentsMargins(
        DesignSystem::layout().px24(), DesignSystem::layout().px24(), DesignSystem::layout().px24(),
        DesignSystem::compactLayout().px12());
    for (auto button : {
             d->openRephraseButton,
             d->openTranslteButton,
             d->openInsertButton,
             d->openExpandButton,
             d->openShortenButton,
             d->openGenerateButton,
             d->openSummarizeButton,
         }) {
        button->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        button->setTextColor(DesignSystem::color().onPrimary());
    }

    for (auto page : {
             d->rephrasePage,
         }) {
        page->setBackgroundColor(DesignSystem::color().primary());
        page->setTextColor(DesignSystem::color().onPrimary());
        page->layout()->setSpacing(DesignSystem::compactLayout().px16());

        page->backButton->setBackgroundColor(DesignSystem::color().primary());
        page->backButton->setTextColor(DesignSystem::color().onPrimary());
        page->titleLabel->setBackgroundColor(DesignSystem::color().primary());
        page->titleLabel->setTextColor(DesignSystem::color().onPrimary());
    }

    for (auto textField : std::vector<TextField*>{
             d->rephraseSourceText,
             d->rephraseStyleText,
             d->rephraseResultText,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onPrimary());
        textField->setTextColor(DesignSystem::color().onPrimary());
    }
    for (auto button : {
             d->rephraseButton,
             d->rephraseInsertButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }
    for (auto buttonsLayout : {
             d->rephraseButtonsLayout,
         }) {
        buttonsLayout->setContentsMargins(isLeftToRight() ? 0.0 : DesignSystem::layout().px8(), 0.0,
                                          isLeftToRight() ? DesignSystem::layout().px8() : 0.0,
                                          0.0);
    }

    d->availableWordsLabel->setBackgroundColor(DesignSystem::color().primary());
    d->availableWordsLabel->setTextColor(ColorHelper::transparent(
        DesignSystem::color().onPrimary(), DesignSystem::inactiveTextOpacity()));
    d->availableWordsLabel->setContentsMarginsF(DesignSystem::label().margins());
}

} // namespace Ui
