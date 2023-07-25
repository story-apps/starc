#include "ai_assistant_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QBoxLayout>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTimer>


namespace Ui {

using ShadowWodget = Shadow;

namespace {

/**
 * @brief Класс странички с конкретной функцией ИИ помощника
 */
class Page : public QScrollArea
{
public:
    explicit Page(QWidget* _parent = nullptr)
        : QScrollArea(_parent)
        , backButton(new IconButton(_parent))
        , titleLabel(new Subtitle2Label(_parent))
        , contentsLayout(new QVBoxLayout)
    {
        QPalette palette;
        palette.setColor(QPalette::Base, Qt::transparent);
        palette.setColor(QPalette::Window, Qt::transparent);
        setPalette(palette);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBar(new ScrollBar);

        new ShadowWodget(Qt::BottomEdge, this);

        backButton->setIcon(u8"\U000F0141");

        contentsLayout->setContentsMargins({});
        contentsLayout->setSpacing(0);
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(backButton);
        titleLayout->addWidget(titleLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
        contentsLayout->addLayout(titleLayout);

        QWidget* contentWidget = new QWidget;
        setWidget(contentWidget);
        setWidgetResizable(true);
        contentWidget->setLayout(contentsLayout);
    }

    IconButton* backButton = nullptr;
    Subtitle2Label* titleLabel = nullptr;
    QVBoxLayout* contentsLayout = nullptr;
};

} // namespace

class AiAssistantView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    bool isReadOnly = false;

    StackWidget* pages = nullptr;

    Widget* buttonsPage = nullptr;
    Button* openRephraseButton = nullptr;
    Button* openExpandButton = nullptr;
    Button* openShortenButton = nullptr;
    Button* openInsertButton = nullptr;
    Button* openSummarizeButton = nullptr;
    Button* openTranslateButton = nullptr;
    Button* openGenerateSynopsisButton = nullptr;
    Button* openGenerateButton = nullptr;

    Page* rephrasePage = nullptr;
    TextField* rephraseSourceText = nullptr;
    TextField* rephraseStyleText = nullptr;
    TextField* rephraseResultText = nullptr;
    Button* rephraseInsertButton = nullptr;
    Button* rephraseButton = nullptr;
    QHBoxLayout* rephraseButtonsLayout = nullptr;

    Page* expandPage = nullptr;
    TextField* expandSourceText = nullptr;
    TextField* expandResultText = nullptr;
    Button* expandInsertButton = nullptr;
    Button* expandButton = nullptr;
    QHBoxLayout* expandButtonsLayout = nullptr;

    Page* shortenPage = nullptr;
    TextField* shortenSourceText = nullptr;
    TextField* shortenResultText = nullptr;
    Button* shortenInsertButton = nullptr;
    Button* shortenButton = nullptr;
    QHBoxLayout* shortenButtonsLayout = nullptr;

    Page* insertPage = nullptr;
    TextField* insertAfterText = nullptr;
    TextField* insertBeforeText = nullptr;
    TextField* insertResultText = nullptr;
    Button* insertInsertButton = nullptr;
    Button* insertButton = nullptr;
    QHBoxLayout* insertButtonsLayout = nullptr;

    Page* summarizePage = nullptr;
    TextField* summarizeSourceText = nullptr;
    TextField* summarizeResultText = nullptr;
    Button* summarizeInsertButton = nullptr;
    Button* summarizeButton = nullptr;
    QHBoxLayout* summarizeButtonsLayout = nullptr;

    Page* translatePage = nullptr;
    TextField* translateSourceText = nullptr;
    ComboBox* translateLanguage = nullptr;
    TextField* translateResultText = nullptr;
    Button* translateInsertButton = nullptr;
    Button* translateButton = nullptr;
    QHBoxLayout* translateButtonsLayout = nullptr;

    Page* generateSynopsisPage = nullptr;
    Body1Label* generateSynopsisHintLabel = nullptr;
    Body2Label* generateSynopsisLenghtLabel = nullptr;
    RadioButton* generateSynopsisLengthShort = nullptr;
    RadioButton* generateSynopsisLengthMedium = nullptr;
    RadioButton* generateSynopsisLengthDefault = nullptr;
    TextField* generateSynopsisResultText = nullptr;
    Button* generateSynopsisButton = nullptr;
    QHBoxLayout* generateSynopsisButtonsLayout = nullptr;

    GenerationViewType generationViewType = GenerationViewType::Text;

    Page* generateTextPage = nullptr;
    Body1Label* generateTextPromptHintLabel = nullptr;
    TextField* generateTextPromptText = nullptr;
    Body2Label* generateTextInsertLabel = nullptr;
    RadioButton* generateTextInsertAtBegin = nullptr;
    RadioButton* generateTextInsertAtCursor = nullptr;
    RadioButton* generateTextInsertAtEnd = nullptr;
    Button* generateTextButton = nullptr;
    QHBoxLayout* generateTextButtonsLayout = nullptr;

    Page* generateCharacterPage = nullptr;
    Body1Label* generateCharacterPromptHintLabel = nullptr;
    TextField* generateCharacterPromptText = nullptr;
    CheckBox* generateCharacterPersonalInfo = nullptr;
    CheckBox* generateCharacterPhysique = nullptr;
    CheckBox* generateCharacterLife = nullptr;
    CheckBox* generateCharacterAttitude = nullptr;
    CheckBox* generateCharacterBiography = nullptr;
    CheckBox* generateCharacterPhoto = nullptr;
    Button* generateCharacterButton = nullptr;
    QHBoxLayout* generateCharacterButtonsLayout = nullptr;

    Body2Label* availableWordsLabel = nullptr;
    Body2LinkLabel* buyCreditsLabel = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

AiAssistantView::Implementation::Implementation(QWidget* _parent)
    : pages(new StackWidget(_parent))
    , buttonsPage(new Widget(pages))
    , openRephraseButton(new Button(buttonsPage))
    , openExpandButton(new Button(buttonsPage))
    , openShortenButton(new Button(buttonsPage))
    , openInsertButton(new Button(buttonsPage))
    , openSummarizeButton(new Button(buttonsPage))
    , openTranslateButton(new Button(buttonsPage))
    , openGenerateSynopsisButton(new Button(buttonsPage))
    , openGenerateButton(new Button(buttonsPage))
    //
    , rephrasePage(new Page(pages))
    , rephraseSourceText(new TextField(rephrasePage))
    , rephraseStyleText(new TextField(rephrasePage))
    , rephraseResultText(new TextField(rephrasePage))
    , rephraseInsertButton(new Button(rephrasePage))
    , rephraseButton(new Button(rephrasePage))
    , rephraseButtonsLayout(new QHBoxLayout)
    //
    , expandPage(new Page(pages))
    , expandSourceText(new TextField(expandPage))
    , expandResultText(new TextField(expandPage))
    , expandInsertButton(new Button(expandPage))
    , expandButton(new Button(expandPage))
    , expandButtonsLayout(new QHBoxLayout)
    //
    , shortenPage(new Page(pages))
    , shortenSourceText(new TextField(shortenPage))
    , shortenResultText(new TextField(shortenPage))
    , shortenInsertButton(new Button(shortenPage))
    , shortenButton(new Button(shortenPage))
    , shortenButtonsLayout(new QHBoxLayout)
    //
    , insertPage(new Page(pages))
    , insertAfterText(new TextField(insertPage))
    , insertBeforeText(new TextField(insertPage))
    , insertResultText(new TextField(insertPage))
    , insertInsertButton(new Button(insertPage))
    , insertButton(new Button(insertPage))
    , insertButtonsLayout(new QHBoxLayout)
    //
    , summarizePage(new Page(pages))
    , summarizeSourceText(new TextField(summarizePage))
    , summarizeResultText(new TextField(summarizePage))
    , summarizeInsertButton(new Button(summarizePage))
    , summarizeButton(new Button(summarizePage))
    , summarizeButtonsLayout(new QHBoxLayout)
    //
    , translatePage(new Page(pages))
    , translateSourceText(new TextField(translatePage))
    , translateLanguage(new ComboBox(translatePage))
    , translateResultText(new TextField(translatePage))
    , translateInsertButton(new Button(translatePage))
    , translateButton(new Button(translatePage))
    , translateButtonsLayout(new QHBoxLayout)
    //
    , generateSynopsisPage(new Page(pages))
    , generateSynopsisHintLabel(new Body1Label(generateSynopsisPage))
    , generateSynopsisLenghtLabel(new Body2Label(generateSynopsisPage))
    , generateSynopsisLengthShort(new RadioButton(generateSynopsisPage))
    , generateSynopsisLengthMedium(new RadioButton(generateSynopsisPage))
    , generateSynopsisLengthDefault(new RadioButton(generateSynopsisPage))
    , generateSynopsisResultText(new TextField(summarizePage))
    , generateSynopsisButton(new Button(summarizePage))
    , generateSynopsisButtonsLayout(new QHBoxLayout)
    //
    , generateTextPage(new Page(pages))
    , generateTextPromptHintLabel(new Body1Label(generateTextPage))
    , generateTextPromptText(new TextField(generateTextPage))
    , generateTextInsertLabel(new Body2Label(generateTextPage))
    , generateTextInsertAtBegin(new RadioButton(generateTextPage))
    , generateTextInsertAtCursor(new RadioButton(generateTextPage))
    , generateTextInsertAtEnd(new RadioButton(generateTextPage))
    , generateTextButton(new Button(generateTextPage))
    , generateTextButtonsLayout(new QHBoxLayout)
    //
    , generateCharacterPage(new Page(pages))
    , generateCharacterPromptHintLabel(new Body1Label(generateCharacterPage))
    , generateCharacterPromptText(new TextField(generateCharacterPage))
    , generateCharacterPersonalInfo(new CheckBox(generateCharacterPage))
    , generateCharacterPhysique(new CheckBox(generateCharacterPage))
    , generateCharacterLife(new CheckBox(generateCharacterPage))
    , generateCharacterAttitude(new CheckBox(generateCharacterPage))
    , generateCharacterBiography(new CheckBox(generateCharacterPage))
    , generateCharacterPhoto(new CheckBox(generateCharacterPage))
    , generateCharacterButton(new Button(generateCharacterPage))
    , generateCharacterButtonsLayout(new QHBoxLayout)
    //
    , availableWordsLabel(new Body2Label(_parent))
    , buyCreditsLabel(new Body2LinkLabel(_parent))
    , buttonsLayout(new QHBoxLayout)
{
    pages->setAnimationType(StackWidget::AnimationType::Slide);
    pages->setCurrentWidget(buttonsPage);
    pages->addWidget(rephrasePage);
    pages->addWidget(expandPage);
    pages->addWidget(shortenPage);
    pages->addWidget(insertPage);
    pages->addWidget(summarizePage);
    pages->addWidget(translatePage);
    pages->addWidget(generateSynopsisPage);
    pages->addWidget(generateTextPage);
    pages->addWidget(generateCharacterPage);

    {
        openRephraseButton->setIcon(u8"\U000F0456");
        openExpandButton->setIcon(u8"\U000F084F");
        openShortenButton->setIcon(u8"\U000F084D");
        openInsertButton->setIcon(u8"\U000F0B33");
        openSummarizeButton->setIcon(u8"\U000F0DD0");
        openTranslateButton->setIcon(u8"\U000F05CA");
        openGenerateSynopsisButton->setIcon(u8"\U000F021A");
        openGenerateButton->setIcon(u8"\U000F027F");
        for (auto button : {
                 openRephraseButton,
                 openExpandButton,
                 openShortenButton,
                 openInsertButton,
                 openSummarizeButton,
                 openTranslateButton,
                 openGenerateSynopsisButton,
                 openGenerateButton,
             }) {
            button->setFlat(true);
        }
        openGenerateSynopsisButton->hide();

        auto layout = new QVBoxLayout(buttonsPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(openRephraseButton);
        layout->addWidget(openExpandButton);
        layout->addWidget(openShortenButton);
        layout->addWidget(openInsertButton);
        layout->addWidget(openSummarizeButton);
        layout->addWidget(openTranslateButton);
        layout->addWidget(openGenerateSynopsisButton);
        layout->addWidget(openGenerateButton);
        layout->addStretch();
    }

    {
        rephraseSourceText->setEnterMakesNewLine(true);
        rephraseSourceText->setWordCount("0/1000");
        rephraseStyleText->setEnterMakesNewLine(true);
        rephraseStyleText->setWordCount("0/1000");
        rephraseResultText->setEnterMakesNewLine(true);
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

    {
        expandSourceText->setEnterMakesNewLine(true);
        expandSourceText->setWordCount("0/1000");
        expandResultText->setEnterMakesNewLine(true);
        expandResultText->hide();
        expandInsertButton->hide();
        expandButtonsLayout->setContentsMargins({});
        expandButtonsLayout->setSpacing(0);
        expandButtonsLayout->addStretch();
        expandButtonsLayout->addWidget(expandInsertButton);
        expandButtonsLayout->addWidget(expandButton);


        auto layout = expandPage->contentsLayout;
        layout->addWidget(expandSourceText);
        layout->addWidget(expandResultText);
        layout->addLayout(expandButtonsLayout);
        layout->addStretch();
    }

    {
        shortenSourceText->setEnterMakesNewLine(true);
        shortenSourceText->setWordCount("0/1000");
        shortenResultText->setEnterMakesNewLine(true);
        shortenResultText->hide();
        shortenInsertButton->hide();
        shortenButtonsLayout->setContentsMargins({});
        shortenButtonsLayout->setSpacing(0);
        shortenButtonsLayout->addStretch();
        shortenButtonsLayout->addWidget(shortenInsertButton);
        shortenButtonsLayout->addWidget(shortenButton);


        auto layout = shortenPage->contentsLayout;
        layout->addWidget(shortenSourceText);
        layout->addWidget(shortenResultText);
        layout->addLayout(shortenButtonsLayout);
        layout->addStretch();
    }

    {
        insertAfterText->setEnterMakesNewLine(true);
        insertAfterText->setWordCount("0/1000");
        insertBeforeText->setEnterMakesNewLine(true);
        insertBeforeText->setWordCount("0/1000");
        insertResultText->setEnterMakesNewLine(true);
        insertResultText->hide();
        insertInsertButton->hide();
        insertButtonsLayout->setContentsMargins({});
        insertButtonsLayout->setSpacing(0);
        insertButtonsLayout->addStretch();
        insertButtonsLayout->addWidget(insertInsertButton);
        insertButtonsLayout->addWidget(insertButton);


        auto layout = insertPage->contentsLayout;
        layout->addWidget(insertAfterText);
        layout->addWidget(insertBeforeText);
        layout->addWidget(insertResultText);
        layout->addLayout(insertButtonsLayout);
        layout->addStretch();
    }

    {
        summarizeSourceText->setEnterMakesNewLine(true);
        summarizeSourceText->setWordCount("0/1000");
        summarizeResultText->setEnterMakesNewLine(true);
        summarizeResultText->hide();
        summarizeInsertButton->hide();
        summarizeButtonsLayout->setContentsMargins({});
        summarizeButtonsLayout->setSpacing(0);
        summarizeButtonsLayout->addStretch();
        summarizeButtonsLayout->addWidget(summarizeInsertButton);
        summarizeButtonsLayout->addWidget(summarizeButton);


        auto layout = summarizePage->contentsLayout;
        layout->addWidget(summarizeSourceText);
        layout->addWidget(summarizeResultText);
        layout->addLayout(summarizeButtonsLayout);
        layout->addStretch();
    }

    {
        translateSourceText->setEnterMakesNewLine(true);
        translateSourceText->setWordCount("0/1000");
        translateLanguage->setPopupMaxItems(10);
        translateResultText->setEnterMakesNewLine(true);
        translateResultText->hide();
        translateInsertButton->hide();
        translateButtonsLayout->setContentsMargins({});
        translateButtonsLayout->setSpacing(0);
        translateButtonsLayout->addStretch();
        translateButtonsLayout->addWidget(translateInsertButton);
        translateButtonsLayout->addWidget(translateButton);


        auto layout = translatePage->contentsLayout;
        layout->addWidget(translateSourceText);
        layout->addWidget(translateLanguage);
        layout->addWidget(translateResultText);
        layout->addLayout(translateButtonsLayout);
        layout->addStretch();
    }

    {
        generateSynopsisResultText->setEnterMakesNewLine(true);
        generateSynopsisResultText->hide();
        generateSynopsisLengthShort->setChecked(true);
        auto lengthButtonsGroup = new RadioButtonGroup(generateSynopsisPage);
        lengthButtonsGroup->add(generateSynopsisLengthShort);
        lengthButtonsGroup->add(generateSynopsisLengthMedium);
        lengthButtonsGroup->add(generateSynopsisLengthDefault);
        generateSynopsisButtonsLayout->setContentsMargins({});
        generateSynopsisButtonsLayout->setSpacing(0);
        generateSynopsisButtonsLayout->addStretch();
        generateSynopsisButtonsLayout->addWidget(generateSynopsisButton);


        auto layout = generateSynopsisPage->contentsLayout;
        layout->addWidget(generateSynopsisHintLabel);
        layout->addWidget(generateSynopsisLenghtLabel);
        layout->addWidget(generateSynopsisLengthShort);
        layout->addWidget(generateSynopsisLengthMedium);
        layout->addWidget(generateSynopsisLengthDefault);
        layout->addWidget(generateSynopsisResultText);
        layout->addLayout(generateSynopsisButtonsLayout);
        layout->addStretch();
    }

    {
        generateTextPromptText->setEnterMakesNewLine(true);
        generateTextPromptText->setWordCount("0/1000");
        generateTextInsertAtCursor->setChecked(true);
        auto insertButtonsGroup = new RadioButtonGroup(generateTextPage);
        insertButtonsGroup->add(generateTextInsertAtBegin);
        insertButtonsGroup->add(generateTextInsertAtCursor);
        insertButtonsGroup->add(generateTextInsertAtEnd);
        generateTextButtonsLayout->setContentsMargins({});
        generateTextButtonsLayout->setSpacing(0);
        generateTextButtonsLayout->addStretch();
        generateTextButtonsLayout->addWidget(generateTextButton);


        auto layout = generateTextPage->contentsLayout;
        layout->addWidget(generateTextPromptHintLabel);
        layout->addWidget(generateTextPromptText);
        layout->addWidget(generateTextInsertLabel);
        layout->addWidget(generateTextInsertAtBegin);
        layout->addWidget(generateTextInsertAtCursor);
        layout->addWidget(generateTextInsertAtEnd);
        layout->addLayout(generateTextButtonsLayout);
        layout->addStretch();
    }

    {
        generateCharacterPromptText->setEnterMakesNewLine(true);
        generateCharacterPromptText->setWordCount("0/1000");
        generateCharacterPersonalInfo->setChecked(true);
        generateCharacterPhysique->setChecked(true);
        generateCharacterLife->setChecked(true);
        generateCharacterAttitude->setChecked(true);
        generateCharacterBiography->setChecked(true);
        generateCharacterPhoto->setChecked(true);
        generateCharacterButtonsLayout->setContentsMargins({});
        generateCharacterButtonsLayout->setSpacing(0);
        generateCharacterButtonsLayout->addStretch();
        generateCharacterButtonsLayout->addWidget(generateCharacterButton);


        auto layout = generateCharacterPage->contentsLayout;
        layout->addWidget(generateCharacterPromptHintLabel);
        layout->addWidget(generateCharacterPromptText);
        layout->addWidget(generateCharacterPhoto);
        layout->addWidget(generateCharacterPersonalInfo);
        layout->addWidget(generateCharacterPhysique);
        layout->addWidget(generateCharacterLife);
        layout->addWidget(generateCharacterAttitude);
        layout->addWidget(generateCharacterBiography);
        layout->addLayout(generateCharacterButtonsLayout);
        layout->addStretch();
    }

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(availableWordsLabel);
    buttonsLayout->addWidget(buyCreditsLabel);
    buttonsLayout->addStretch();
}


// ****


AiAssistantView::AiAssistantView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->pages, 1);
    layout->addLayout(d->buttonsLayout);

    connect(d->openRephraseButton, &Button::clicked, this, [this] {
        d->rephraseSourceText->clear();
        d->rephraseStyleText->clear();
        d->rephraseResultText->hide();
        d->rephraseInsertButton->hide();
        d->pages->setCurrentWidget(d->rephrasePage);
        QTimer::singleShot(d->pages->animationDuration(), d->rephraseSourceText,
                           qOverload<>(&TextField::setFocus));
    });
    connect(d->openExpandButton, &Button::clicked, this, [this] {
        d->expandSourceText->clear();
        d->expandResultText->hide();
        d->expandInsertButton->hide();
        d->pages->setCurrentWidget(d->expandPage);
        QTimer::singleShot(d->pages->animationDuration(), d->expandSourceText,
                           qOverload<>(&TextField::setFocus));
    });
    connect(d->openShortenButton, &Button::clicked, this, [this] {
        d->shortenSourceText->clear();
        d->shortenResultText->hide();
        d->shortenInsertButton->hide();
        d->pages->setCurrentWidget(d->shortenPage);
        QTimer::singleShot(d->pages->animationDuration(), d->shortenSourceText,
                           qOverload<>(&TextField::setFocus));
    });
    connect(d->openInsertButton, &Button::clicked, this, [this] {
        d->insertAfterText->clear();
        d->insertBeforeText->clear();
        d->insertResultText->hide();
        d->insertInsertButton->hide();
        d->pages->setCurrentWidget(d->insertPage);
        QTimer::singleShot(d->pages->animationDuration(), d->insertAfterText,
                           qOverload<>(&TextField::setFocus));
    });
    connect(d->openSummarizeButton, &Button::clicked, this, [this] {
        d->summarizeSourceText->clear();
        d->summarizeResultText->hide();
        d->summarizeInsertButton->hide();
        d->pages->setCurrentWidget(d->summarizePage);
        QTimer::singleShot(d->pages->animationDuration(), d->summarizeSourceText,
                           qOverload<>(&TextField::setFocus));
    });
    connect(d->openTranslateButton, &Button::clicked, this, [this] {
        d->translateSourceText->clear();
        d->translateLanguage->clear();
        d->translateResultText->hide();
        d->translateInsertButton->hide();
        d->pages->setCurrentWidget(d->translatePage);
        QTimer::singleShot(d->pages->animationDuration(), d->translateSourceText,
                           qOverload<>(&TextField::setFocus));
    });
    connect(d->openGenerateSynopsisButton, &Button::clicked, this, [this] {
        d->generateSynopsisResultText->hide();
        d->pages->setCurrentWidget(d->generateSynopsisPage);
        QTimer::singleShot(d->pages->animationDuration(), d->generateSynopsisButton,
                           qOverload<>(&Button::setFocus));
    });
    connect(d->openGenerateButton, &Button::clicked, this, [this] {
        switch (d->generationViewType) {
        case GenerationViewType::Text: {
            d->pages->setCurrentWidget(d->generateTextPage);
            QTimer::singleShot(d->pages->animationDuration(), d->generateTextPromptText,
                               qOverload<>(&TextField::setFocus));
            break;
        }

        case GenerationViewType::CharacterInformation: {
            d->pages->setCurrentWidget(d->generateCharacterPage);
            QTimer::singleShot(d->pages->animationDuration(), d->generateCharacterPromptText,
                               qOverload<>(&TextField::setFocus));
            break;
        }
        }
    });
    for (auto page : {
             d->rephrasePage,
             d->expandPage,
             d->shortenPage,
             d->insertPage,
             d->summarizePage,
             d->translatePage,
             d->generateSynopsisPage,
             d->generateTextPage,
             d->generateCharacterPage,
         }) {
        connect(page->backButton, &IconButton::clicked, this,
                [this] { d->pages->setCurrentWidget(d->buttonsPage); });
        connect(page->titleLabel, &Subtitle2Label::clicked, this,
                [this] { d->pages->setCurrentWidget(d->buttonsPage); });
    }
    auto updateWordCounter = [](TextField* _textField) {
        const auto wordCount = TextHelper::wordsCount(_textField->text());
        _textField->setWordCount(QString("%1/1000").arg(wordCount));
        return wordCount;
    };
    auto updateResultWordCounter = [](TextField* _textField) {
        _textField->setWordCount(QString::number(TextHelper::wordsCount(_textField->text())));
    };
    //
    auto updateRephraseWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->rephraseSourceText);
        const auto styleTextWordCount = updateWordCounter(d->rephraseStyleText);
        d->rephraseButton->setEnabled(sourceTextWordCount <= 1000 && styleTextWordCount <= 1000);
    };
    connect(d->rephraseSourceText, &TextField::textChanged, this, updateRephraseWordCounters);
    connect(d->rephraseStyleText, &TextField::textChanged, this, updateRephraseWordCounters);
    connect(d->rephraseResultText, &TextField::textChanged, this,
            [textField = d->rephraseResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    connect(d->rephraseButton, &Button::clicked, this, [this] {
        emit rephraseRequested(d->rephraseSourceText->text(), d->rephraseStyleText->text());
    });
    connect(d->rephraseInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->rephraseResultText->text()); });
    //
    auto updateExpandWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->expandSourceText);
        d->expandButton->setEnabled(sourceTextWordCount <= 1000);
    };
    connect(d->expandSourceText, &TextField::textChanged, this, updateExpandWordCounters);
    connect(d->expandResultText, &TextField::textChanged, this,
            [textField = d->expandResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    connect(d->expandButton, &Button::clicked, this,
            [this] { emit expandRequested(d->expandSourceText->text()); });
    connect(d->expandInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->expandResultText->text()); });
    //
    auto updateShortenWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->shortenSourceText);
        d->shortenButton->setEnabled(sourceTextWordCount <= 1000);
    };
    connect(d->shortenSourceText, &TextField::textChanged, this, updateShortenWordCounters);
    connect(d->shortenResultText, &TextField::textChanged, this,
            [textField = d->shortenResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    connect(d->shortenButton, &Button::clicked, this,
            [this] { emit shortenRequested(d->shortenSourceText->text()); });
    connect(d->shortenInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->shortenResultText->text()); });
    //
    auto updateInsertWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->insertAfterText);
        const auto styleTextWordCount = updateWordCounter(d->insertBeforeText);
        d->insertButton->setEnabled(sourceTextWordCount <= 1000 && styleTextWordCount <= 1000);
    };
    connect(d->insertAfterText, &TextField::textChanged, this, updateInsertWordCounters);
    connect(d->insertBeforeText, &TextField::textChanged, this, updateInsertWordCounters);
    connect(d->insertResultText, &TextField::textChanged, this,
            [textField = d->insertResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    connect(d->insertButton, &Button::clicked, this, [this] {
        emit insertRequested(d->insertAfterText->text(), d->insertBeforeText->text());
    });
    connect(d->insertInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->insertResultText->text()); });
    //
    auto updateSummarizeWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->summarizeSourceText);
        d->summarizeButton->setEnabled(sourceTextWordCount <= 1000);
    };
    connect(d->summarizeSourceText, &TextField::textChanged, this, updateSummarizeWordCounters);
    connect(d->summarizeResultText, &TextField::textChanged, this,
            [textField = d->summarizeResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    connect(d->summarizeButton, &Button::clicked, this,
            [this] { emit summarizeRequested(d->summarizeSourceText->text()); });
    connect(d->summarizeInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->summarizeResultText->text()); });
    //
    auto updateTranslateWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->translateSourceText);
        d->translateButton->setEnabled(sourceTextWordCount <= 1000);
    };
    connect(d->translateSourceText, &TextField::textChanged, this, updateTranslateWordCounters);
    connect(d->translateLanguage, &TextField::textChanged, this, updateTranslateWordCounters);
    connect(d->translateResultText, &TextField::textChanged, this,
            [textField = d->translateResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    connect(d->translateButton, &Button::clicked, this, [this] {
        emit translateRequested(d->translateSourceText->text(),
                                d->translateLanguage->currentIndex().data(Qt::UserRole).toString());
    });
    connect(d->translateInsertButton, &Button::clicked, this,
            [this] { emit insertTextRequested(d->translateResultText->text()); });
    //
    connect(d->generateSynopsisButton, &Button::clicked, this, [this] {
        emit generateSynopsisRequested(
            d->generateSynopsisLengthShort->isChecked()
                ? 20
                : (d->generateSynopsisLengthMedium->isChecked() ? 45 : -1));
    });
    connect(d->generateSynopsisResultText, &TextField::textChanged, this,
            [textField = d->generateSynopsisResultText, updateResultWordCounter] {
                updateResultWordCounter(textField);
            });
    //
    auto updateGenerateTextWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->generateTextPromptText);
        d->generateTextButton->setEnabled(sourceTextWordCount <= 1000);
    };
    connect(d->generateTextPromptText, &TextField::textChanged, this,
            updateGenerateTextWordCounters);
    connect(d->generateTextButton, &Button::clicked, this,
            [this] { emit generateTextRequested(d->generateTextPromptText->text()); });
    //
    auto updateGenerateCharacterWordCounters = [this, updateWordCounter] {
        const auto sourceTextWordCount = updateWordCounter(d->generateCharacterPromptText);
        d->generateCharacterButton->setEnabled(sourceTextWordCount <= 1000);
    };
    connect(d->generateCharacterPromptText, &TextField::textChanged, this,
            updateGenerateCharacterWordCounters);
    connect(d->generateCharacterButton, &Button::clicked, this, [this] {
        emit generateCharacterRequested(
            d->generateCharacterPromptText->text(), d->generateCharacterPersonalInfo->isChecked(),
            d->generateCharacterPhysique->isChecked(), d->generateCharacterLife->isChecked(),
            d->generateCharacterAttitude->isChecked(), d->generateCharacterBiography->isChecked(),
            d->generateCharacterPhoto->isChecked());
    });

    connect(d->buyCreditsLabel, &Body1LinkLabel::clicked, this,
            &AiAssistantView::buyCreditsPressed);
}

AiAssistantView::~AiAssistantView() = default;

bool AiAssistantView::isReadOnly() const
{
    return d->isReadOnly;
}

void AiAssistantView::setReadOnly(bool _readOnly)
{
    if (d->isReadOnly == _readOnly) {
        return;
    }

    d->isReadOnly = _readOnly;
    for (auto button : std::vector<QWidget*>{
             d->buttonsPage,
             d->rephrasePage,
             d->expandPage,
             d->shortenPage,
             d->insertPage,
             d->summarizePage,
             d->translatePage,
             d->generateSynopsisPage,
             d->generateTextPage,
             d->generateCharacterPage,
         }) {
        button->setEnabled(!_readOnly);
    }
}

void AiAssistantView::setInsertionAvailable(bool _available)
{
    for (auto button : {
             d->rephraseInsertButton,
             d->expandInsertButton,
             d->shortenInsertButton,
             d->insertInsertButton,
             d->summarizeInsertButton,
             d->translateInsertButton,
         }) {
        button->setEnabled(_available);
    }
}

void AiAssistantView::setSynopsisGenerationAvaiable(bool _available)
{
    d->openGenerateSynopsisButton->setVisible(_available);
}

void AiAssistantView::setGenerationSynopsisPromptHint(const QString& _hint)
{
    d->generateSynopsisHintLabel->setText(_hint);
}

void AiAssistantView::setGenerationViewType(GenerationViewType _type)
{
    d->generationViewType = _type;
}

void AiAssistantView::setGenerationPromptHint(const QString& _hint)
{
    switch (d->generationViewType) {
    case GenerationViewType::Text: {
        d->generateTextPromptHintLabel->setText(_hint);
        break;
    }

    case GenerationViewType::CharacterInformation: {
        d->generateCharacterPromptHintLabel->setText(_hint);
        break;
    }
    }
}

void AiAssistantView::setGenerationPrompt(const QString& _prompt)
{
    switch (d->generationViewType) {
    case GenerationViewType::Text: {
        d->generateTextPromptText->setText(_prompt);
        break;
    }

    case GenerationViewType::CharacterInformation: {
        d->generateCharacterPromptText->setText(_prompt);
        break;
    }
    }
}

AiAssistantView::TextInsertPosition AiAssistantView::textInsertPosition() const
{
    if (d->generateTextInsertAtBegin->isChecked()) {
        return TextInsertPosition::AtBeginning;
    } else if (d->generateTextInsertAtCursor->isChecked()) {
        return TextInsertPosition::AtCursorPosition;
    } else {
        return TextInsertPosition::AtEnd;
    }
}

void AiAssistantView::setRephraseResult(const QString& _text)
{
    d->rephraseResultText->setText(_text);
    d->rephraseResultText->show();
    d->rephraseInsertButton->show();
}

void AiAssistantView::setExpandResult(const QString& _text)
{
    d->expandResultText->setText(_text);
    d->expandResultText->show();
    d->expandInsertButton->show();
}

void AiAssistantView::setShortenResult(const QString& _text)
{
    d->shortenResultText->setText(_text);
    d->shortenResultText->show();
    d->shortenInsertButton->show();
}

void AiAssistantView::setInsertResult(const QString& _text)
{
    d->insertResultText->setText(_text);
    d->insertResultText->show();
    d->insertInsertButton->show();
}

void AiAssistantView::setSummarizeResult(const QString& _text)
{
    d->summarizeResultText->setText(_text);
    d->summarizeResultText->show();
    d->summarizeInsertButton->show();
}

void AiAssistantView::setTransateResult(const QString& _text)
{
    d->translateResultText->setText(_text);
    d->translateResultText->show();
    d->translateInsertButton->show();
}

void AiAssistantView::setGenerateSynopsisResult(const QString& _text)
{
    d->generateSynopsisResultText->setText(_text);
    d->generateSynopsisResultText->show();
}

void AiAssistantView::setAvailableWords(int _availableWords)
{
    d->availableWordsLabel->setText(_availableWords > 0
                                        ? tr("%n word(s) available", nullptr, _availableWords)
                                        : tr("No words available"));
}

void AiAssistantView::updateTranslations()
{
    d->openRephraseButton->setText(tr("Rephrase"));
    d->openExpandButton->setText(tr("Expand"));
    d->openShortenButton->setText(tr("Shorten"));
    d->openInsertButton->setText(tr("Insert"));
    d->openSummarizeButton->setText(tr("Summarize"));
    d->openTranslateButton->setText(tr("Translate"));
    d->openGenerateSynopsisButton->setText(tr("Generate synopsis"));
    d->openGenerateButton->setText(tr("Generate"));

    for (auto page : {
             d->rephrasePage,
             d->expandPage,
             d->shortenPage,
             d->insertPage,
             d->summarizePage,
             d->translatePage,
             d->generateSynopsisPage,
             d->generateTextPage,
             d->generateCharacterPage,
         }) {
        page->backButton->setToolTip(tr("Go back to list of assistant functions"));
    }
    d->rephrasePage->titleLabel->setText(tr("Rephrase"));
    d->rephraseSourceText->setLabel(tr("Text to rephrase"));
    d->rephraseStyleText->setLabel(tr("Rephrase in style of"));
    d->rephraseStyleText->setHelper(tr("Keep empty to avoid style changing"));
    d->rephraseResultText->setLabel(tr("Rephrased text"));
    d->rephraseButton->setText(tr("Rephrase"));
    d->rephraseInsertButton->setText(tr("Insert"));
    d->expandPage->titleLabel->setText(tr("Expand"));
    d->expandSourceText->setLabel(tr("Text to expand"));
    d->expandResultText->setLabel(tr("Expanded text"));
    d->expandButton->setText(tr("Expand"));
    d->expandInsertButton->setText(tr("Insert"));
    d->shortenPage->titleLabel->setText(tr("Shorten"));
    d->shortenSourceText->setLabel(tr("Text to shorten"));
    d->shortenResultText->setLabel(tr("Shortened text"));
    d->shortenButton->setText(tr("Shorten"));
    d->shortenInsertButton->setText(tr("Insert"));
    d->insertPage->titleLabel->setText(tr("Insert"));
    d->insertAfterText->setLabel(tr("Text before inserted"));
    d->insertBeforeText->setLabel(tr("Text after inserted"));
    d->insertResultText->setLabel(tr("Inserted text"));
    d->insertButton->setText(tr("Generate"));
    d->insertInsertButton->setText(tr("Insert"));
    d->summarizePage->titleLabel->setText(tr("Summarize"));
    d->summarizeSourceText->setLabel(tr("Text to summarize"));
    d->summarizeResultText->setLabel(tr("Summarized text"));
    d->summarizeButton->setText(tr("Summarize"));
    d->summarizeInsertButton->setText(tr("Insert"));
    d->translatePage->titleLabel->setText(tr("Translate"));
    d->translateSourceText->setLabel(tr("Text to translate"));
    d->translateLanguage->setLabel(tr("Translate to"));
    {
        const QHash<QString, QString> languagesToCodes = {
            { tr("Afrikaans"), "af" },
            { tr("Albanian"), "sq" },
            { tr("Amharic"), "am" },
            { tr("Arabic"), "ar" },
            { tr("Armenian"), "hy" },
            { tr("Assamese"), "as" },
            { tr("Aymara"), "ay" },
            { tr("Azerbaijani"), "az" },
            { tr("Bambara"), "bm" },
            { tr("Basque"), "eu" },
            { tr("Belarusian"), "be" },
            { tr("Bengali"), "bn" },
            { tr("Bhojpuri"), "bho" },
            { tr("Bosnian"), "bs" },
            { tr("Bulgarian"), "bg" },
            { tr("Catalan"), "ca" },
            { tr("Cebuano"), "ceb" },
            { tr("Chinese (Simplified)"), "zh-CN" },
            { tr("Chinese (Traditional)"), "zh-TW" },
            { tr("Corsican"), "co" },
            { tr("Croatian"), "hr" },
            { tr("Czech"), "cs" },
            { tr("Danish"), "da" },
            { tr("Dhivehi"), "dv" },
            { tr("Dogri"), "doi" },
            { tr("Dutch"), "nl" },
            { tr("English"), "en" },
            { tr("Esperanto"), "eo" },
            { tr("Estonian"), "et" },
            { tr("Ewe"), "ee" },
            { tr("Filipino (Tagalog)"), "fil" },
            { tr("Finnish"), "fi" },
            { tr("French"), "fr" },
            { tr("Frisian"), "fy" },
            { tr("Galician"), "gl" },
            { tr("Georgian"), "ka" },
            { tr("German"), "de" },
            { tr("Greek"), "el" },
            { tr("Guarani"), "gn" },
            { tr("Gujarati"), "gu" },
            { tr("Haitian Creole"), "ht" },
            { tr("Hausa"), "ha" },
            { tr("Hawaiian"), "haw" },
            { tr("Hebrew"), "he" },
            { tr("Hindi"), "hi" },
            { tr("Hmong"), "hmn" },
            { tr("Hungarian"), "hu" },
            { tr("Icelandic"), "is" },
            { tr("Igbo"), "ig" },
            { tr("Ilocano"), "ilo" },
            { tr("Indonesian"), "id" },
            { tr("Irish"), "ga" },
            { tr("Italian"), "it" },
            { tr("Japanese"), "ja" },
            { tr("Javanese"), "jv" },
            { tr("Kannada"), "kn" },
            { tr("Kazakh"), "kk" },
            { tr("Khmer"), "km" },
            { tr("Kinyarwanda"), "rw" },
            { tr("Konkani"), "gom" },
            { tr("Korean"), "ko" },
            { tr("Krio"), "kri" },
            { tr("Kurdish"), "ku" },
            { tr("Kurdish (Sorani)"), "ckb" },
            { tr("Kyrgyz"), "ky" },
            { tr("Lao"), "lo" },
            { tr("Latin"), "la" },
            { tr("Latvian"), "lv" },
            { tr("Lingala"), "ln" },
            { tr("Lithuanian"), "lt" },
            { tr("Luganda"), "lg" },
            { tr("Luxembourgish"), "lb" },
            { tr("Macedonian"), "mk" },
            { tr("Maithili"), "mai" },
            { tr("Malagasy"), "mg" },
            { tr("Malay"), "ms" },
            { tr("Malayalam"), "ml" },
            { tr("Maltese"), "mt" },
            { tr("Maori"), "mi" },
            { tr("Marathi"), "mr" },
            { tr("Meiteilon (Manipuri)"), "mni-Mtei" },
            { tr("Mizo"), "lus" },
            { tr("Mongolian"), "mn" },
            { tr("Myanmar (Burmese)"), "my" },
            { tr("Nepali"), "ne" },
            { tr("Norwegian"), "no" },
            { tr("Nyanja (Chichewa)"), "ny" },
            { tr("Odia (Oriya)"), "or" },
            { tr("Oromo"), "om" },
            { tr("Pashto"), "ps" },
            { tr("Persian"), "fa" },
            { tr("Polish"), "pl" },
            { tr("Portuguese (Portugal, Brazil)"), "pt" },
            { tr("Punjabi"), "pa" },
            { tr("Quechua"), "qu" },
            { tr("Romanian"), "ro" },
            { tr("Russian"), "ru" },
            { tr("Samoan"), "sm" },
            { tr("Sanskrit"), "sa" },
            { tr("Scots Gaelic"), "gd" },
            { tr("Sepedi"), "nso" },
            { tr("Serbian"), "sr" },
            { tr("Sesotho"), "st" },
            { tr("Shona"), "sn" },
            { tr("Sindhi"), "sd" },
            { tr("Sinhala (Sinhalese)"), "si" },
            { tr("Slovak"), "sk" },
            { tr("Slovenian"), "sl" },
            { tr("Somali"), "so" },
            { tr("Spanish"), "es" },
            { tr("Sundanese"), "su" },
            { tr("Swahili"), "sw" },
            { tr("Swedish"), "sv" },
            { tr("Tagalog (Filipino)"), "tl" },
            { tr("Tajik"), "tg" },
            { tr("Tamil"), "ta" },
            { tr("Tatar"), "tt" },
            { tr("Telugu"), "te" },
            { tr("Thai"), "th" },
            { tr("Tigrinya"), "ti" },
            { tr("Tsonga"), "ts" },
            { tr("Turkish"), "tr" },
            { tr("Turkmen"), "tk" },
            { tr("Twi (Akan)"), "ak" },
            { tr("Ukrainian"), "uk" },
            { tr("Urdu"), "ur" },
            { tr("Uyghur"), "ug" },
            { tr("Uzbek"), "uz" },
            { tr("Vietnamese"), "vi" },
            { tr("Welsh"), "cy" },
            { tr("Xhosa"), "xh" },
            { tr("Yiddish"), "yi" },
            { tr("Yoruba"), "yo" },
            { tr("Zulu"), "zu" },
        };
        QStringList languages = languagesToCodes.keys();
        std::sort(languages.begin(), languages.end());
        auto languageModel = qobject_cast<QStandardItemModel*>(d->translateLanguage->model());
        if (languageModel == nullptr) {
            languageModel = new QStandardItemModel(d->translateLanguage);
            d->translateLanguage->setModel(languageModel);
        }
        languageModel->clear();
        for (int index = 0; index < languages.size(); ++index) {
            const auto language = languages.at(index);
            const auto languageCode = languagesToCodes.value(language);
            auto languageItem = new QStandardItem(language);
            languageItem->setData(languageCode, Qt::UserRole);
            languageItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            languageModel->appendRow(languageItem);
        }
    }
    d->translateResultText->setLabel(tr("Translated text"));
    d->translateButton->setText(tr("Translate"));
    d->translateInsertButton->setText(tr("Insert"));
    d->generateSynopsisPage->titleLabel->setText(tr("Generate synopsis"));
    d->generateSynopsisLenghtLabel->setText(tr("Synopsis length"));
    d->generateSynopsisLengthShort->setText(tr("short"));
    d->generateSynopsisLengthMedium->setText(tr("medium"));
    d->generateSynopsisLengthDefault->setText(tr("unlimitted"));
    d->generateSynopsisResultText->setLabel(tr("Synopsis"));
    d->generateSynopsisButton->setText(tr("Generate"));
    d->generateTextPage->titleLabel->setText(tr("Generate"));
    d->generateTextPromptText->setLabel(tr("Prompt"));
    d->generateTextInsertLabel->setText(tr("Insert result"));
    d->generateTextInsertAtBegin->setText(tr("at the beginning of the document"));
    d->generateTextInsertAtCursor->setText(tr("at the cursor position"));
    d->generateTextInsertAtEnd->setText(tr("at the end of the document"));
    d->generateTextButton->setText(tr("Generate"));
    d->generateCharacterPage->titleLabel->setText(tr("Generate"));
    d->generateCharacterPromptText->setLabel(tr("Prompt"));
    d->generateCharacterPersonalInfo->setText(tr("Personal info"));
    d->generateCharacterPhysique->setText(tr("Physique"));
    d->generateCharacterLife->setText(tr("Life"));
    d->generateCharacterAttitude->setText(tr("Attitude"));
    d->generateCharacterBiography->setText(tr("Biography"));
    d->generateCharacterPhoto->setText(tr("Photo"));
    d->generateCharacterButton->setText(tr("Generate"));

    d->buyCreditsLabel->setText(tr("purchase"));
}

void AiAssistantView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
             d->openExpandButton,
             d->openShortenButton,
             d->openInsertButton,
             d->openSummarizeButton,
             d->openTranslateButton,
             d->openGenerateSynopsisButton,
             d->openGenerateButton,
         }) {
        button->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        button->setTextColor(DesignSystem::color().onPrimary());
    }

    for (auto page : {
             d->rephrasePage,
             d->expandPage,
             d->shortenPage,
             d->insertPage,
             d->summarizePage,
             d->translatePage,
             d->generateSynopsisPage,
             d->generateTextPage,
             d->generateCharacterPage,
         }) {
        page->contentsLayout->setSpacing(DesignSystem::compactLayout().px16());

        page->backButton->setBackgroundColor(DesignSystem::color().primary());
        page->backButton->setTextColor(DesignSystem::color().onPrimary());
        page->titleLabel->setBackgroundColor(DesignSystem::color().primary());
        page->titleLabel->setTextColor(DesignSystem::color().onPrimary());
    }
    d->generateSynopsisPage->contentsLayout->setSpacing(0);
    d->generateTextPage->contentsLayout->setSpacing(0);
    d->generateCharacterPage->contentsLayout->setSpacing(0);

    for (auto textField : std::vector<TextField*>{
             d->rephraseSourceText,
             d->rephraseStyleText,
             d->rephraseResultText,
             d->expandSourceText,
             d->expandResultText,
             d->shortenSourceText,
             d->shortenResultText,
             d->insertAfterText,
             d->insertBeforeText,
             d->insertResultText,
             d->summarizeSourceText,
             d->summarizeResultText,
             d->translateSourceText,
             d->translateLanguage,
             d->translateResultText,
             d->generateSynopsisResultText,
             d->generateTextPromptText,
             d->generateCharacterPromptText,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onPrimary());
        textField->setTextColor(DesignSystem::color().onPrimary());
    }
    d->translateLanguage->setPopupBackgroundColor(DesignSystem::color().primary());

    for (auto button : {
             d->rephraseButton,
             d->rephraseInsertButton,
             d->expandButton,
             d->expandInsertButton,
             d->shortenButton,
             d->shortenInsertButton,
             d->insertButton,
             d->insertInsertButton,
             d->summarizeButton,
             d->summarizeInsertButton,
             d->translateButton,
             d->translateInsertButton,
             d->generateSynopsisButton,
             d->generateTextButton,
             d->generateCharacterButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    for (auto buttonsLayout : {
             d->rephraseButtonsLayout,
             d->expandButtonsLayout,
             d->shortenButtonsLayout,
             d->insertButtonsLayout,
             d->summarizeButtonsLayout,
             d->translateButtonsLayout,
             d->generateSynopsisButtonsLayout,
             d->generateTextButtonsLayout,
             d->generateCharacterButtonsLayout,
         }) {
        buttonsLayout->setContentsMargins(isLeftToRight() ? 0.0 : DesignSystem::layout().px8(), 0.0,
                                          isLeftToRight() ? DesignSystem::layout().px8() : 0.0,
                                          0.0);
    }

    for (auto widget : std::list<Widget*>{
             d->generateSynopsisHintLabel,
             d->generateSynopsisLenghtLabel,
             d->generateSynopsisLengthShort,
             d->generateSynopsisLengthMedium,
             d->generateSynopsisLengthDefault,
             d->generateTextPromptHintLabel,
             d->generateTextInsertLabel,
             d->generateTextInsertAtBegin,
             d->generateTextInsertAtCursor,
             d->generateTextInsertAtEnd,
             d->generateCharacterPromptHintLabel,
             d->generateCharacterPersonalInfo,
             d->generateCharacterPhysique,
             d->generateCharacterAttitude,
             d->generateCharacterLife,
             d->generateCharacterBiography,
             d->generateCharacterPhoto,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(DesignSystem::color().onPrimary());
    }
    for (auto label : {
             d->generateSynopsisHintLabel,
             d->generateTextPromptHintLabel,
             d->generateCharacterPromptHintLabel,
         }) {
        label->setContentsMargins(DesignSystem::layout().px24(), 0, DesignSystem::layout().px24(),
                                  DesignSystem::compactLayout().px16());
    }
    d->generateSynopsisLenghtLabel->setContentsMargins(DesignSystem::layout().px24(),
                                                       DesignSystem::compactLayout().px4(),
                                                       DesignSystem::layout().px24(), 0);
    d->generateTextInsertLabel->setContentsMargins(DesignSystem::layout().px24(),
                                                   DesignSystem::compactLayout().px4(),
                                                   DesignSystem::layout().px24(), 0);

    d->availableWordsLabel->setBackgroundColor(DesignSystem::color().primary());
    d->availableWordsLabel->setTextColor(ColorHelper::transparent(
        DesignSystem::color().onPrimary(), DesignSystem::inactiveTextOpacity()));
    d->availableWordsLabel->setContentsMargins(
        DesignSystem::layout().px24(), DesignSystem::compactLayout().px16(),
        DesignSystem::layout().px4(), DesignSystem::layout().px24());
    d->buyCreditsLabel->setBackgroundColor(DesignSystem::color().primary());
    d->buyCreditsLabel->setTextColor(DesignSystem::color().accent());
    d->buyCreditsLabel->setContentsMargins(
        DesignSystem::layout().px4(), DesignSystem::compactLayout().px16(),
        DesignSystem::layout().px24(), DesignSystem::layout().px24());
}

} // namespace Ui
