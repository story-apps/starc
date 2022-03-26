#include "logline_generator_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/progress_bar/progress_bar.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/text_helper.h>

#include <QBoxLayout>
#include <QStringListModel>
#include <QTimer>


namespace Ui {

namespace {

/**
 * @brief Страницы генератора
 */
enum Page {
    Character,
    MajorEvent,
    Theme,
    StoryGoal,
    Mpr,
    World,
    Stakes,
    Logline,
};

/**
 * @brief Виджет подсказки
 */
class AdviseWidget : public Widget
{
public:
    explicit AdviseWidget(QWidget* _parent = nullptr)
        : Widget(_parent)
        , icon(new IconsMidLabel(this))
        , title(new H6Label(this))
        , text(new Body2Label(this))
    {
        icon->setIcon(u8"\U000F0335");

        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(icon);
        titleLayout->addWidget(title, 1);

        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(text, 1, Qt::AlignTop);
        setLayout(layout);

        updateTranslations();
        designSystemChangeEvent(nullptr);
    }

    void setText(const QString& _text)
    {
        text->setText(_text);
    }

protected:
    void updateTranslations() override
    {
        title->setText(tr("Hint"));
    }

    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override
    {
        Widget::designSystemChangeEvent(_event);

        for (auto widget : std::vector<Widget*>{
                 this,
                 icon,
                 title,
                 text,
             }) {
            widget->setBackgroundColor(Ui::DesignSystem::color().background());
            widget->setTextColor(Ui::DesignSystem::color().onBackground());
        }

        icon->setContentsMargins(0, 0, Ui::DesignSystem::layout().px8(), 0);
        title->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());
        text->setContentsMargins(Ui::DesignSystem::layout().px4(), 0,
                                 Ui::DesignSystem::layout().px12(), 0);
        text->setFixedHeight(Ui::DesignSystem::layout().px(280));
    }

private:
    IconsMidLabel* icon = nullptr;
    H6Label* title = nullptr;
    Body2Label* text = nullptr;
};
} // namespace

class LoglineGeneratorDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить виджет страницы соответствующий перечислению
     */
    QWidget* page(Page _page) const;

    /**
     * @brief Получить виджет страницы, который необходимо сфокусировать после отображения страницы
     */
    QWidget* pageFocus(Page _page) const;

    /**
     * @brief Проверить всё ли необходимое заполнено на текущей странице и подсветить ошибки
     */
    bool checkCurrentPage();

    /**
     * @brief Построить логлайн
     */
    QString buildLogline() const;


    ProgressBar* progressBar = nullptr;
    StackWidget* content = nullptr;
    Page currentPage = Character;

    Widget* characterPage = nullptr;
    TextField* characterInfo = nullptr;
    ComboBox* characterGender = nullptr;
    QStringListModel* characterGenderModel = nullptr;
    AdviseWidget* characterAdvise = nullptr;

    Widget* majorEventPage = nullptr;
    TextField* majorEvent = nullptr;
    CheckBox* majorEventIncludesMainChanager = nullptr;
    AdviseWidget* majorEventAdvise = nullptr;

    Widget* themePage = nullptr;
    CheckBox* includeTheme = nullptr;
    TextField* theme = nullptr;
    AdviseWidget* themeAdvise = nullptr;

    Widget* storyGoalPage = nullptr;
    TextField* storyGoal = nullptr;
    AdviseWidget* storyGoalAdvise = nullptr;

    Widget* mprPage = nullptr;
    CheckBox* includeMpr = nullptr;
    TextField* mprEvent = nullptr;
    TextField* afterMprEvent = nullptr;
    AdviseWidget* mprAdvise = nullptr;

    Widget* worldPage = nullptr;
    CheckBox* worldHasSpecialRules = nullptr;
    TextField* worldSpecialRules = nullptr;
    AdviseWidget* worldAdvise = nullptr;

    Widget* stakesPage = nullptr;
    CheckBox* includeStakes = nullptr;
    TextField* stakes = nullptr;
    AdviseWidget* stakesAdvise = nullptr;

    Widget* loglinePage = nullptr;
    IconsBigLabel* loglineIcon = nullptr;
    H6Label* loglineTitle = nullptr;
    Body1Label* logline = nullptr;

    Button* backButton = nullptr;
    Button* closeButton = nullptr;
    Button* continueButton = nullptr;
    Button* doneButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

LoglineGeneratorDialog::Implementation::Implementation(QWidget* _parent)
    : progressBar(new ProgressBar(_parent))
    , content(new StackWidget(_parent))
    //
    , characterPage(new Widget(_parent))
    , characterInfo(new TextField(characterPage))
    , characterGender(new ComboBox(_parent))
    , characterGenderModel(new QStringListModel(characterGender))
    , characterAdvise(new AdviseWidget(_parent))
    //
    , majorEventPage(new Widget(_parent))
    , majorEvent(new TextField(_parent))
    , majorEventIncludesMainChanager(new CheckBox(_parent))
    , majorEventAdvise(new AdviseWidget(_parent))
    //
    , themePage(new Widget(_parent))
    , includeTheme(new CheckBox(_parent))
    , theme(new TextField(_parent))
    , themeAdvise(new AdviseWidget(_parent))
    //
    , storyGoalPage(new Widget(_parent))
    , storyGoal(new TextField(_parent))
    , storyGoalAdvise(new AdviseWidget(_parent))
    //
    , mprPage(new Widget(_parent))
    , includeMpr(new CheckBox(_parent))
    , mprEvent(new TextField(_parent))
    , afterMprEvent(new TextField(_parent))
    , mprAdvise(new AdviseWidget(_parent))
    //
    , worldPage(new Widget(_parent))
    , worldHasSpecialRules(new CheckBox(_parent))
    , worldSpecialRules(new TextField(_parent))
    , worldAdvise(new AdviseWidget(_parent))
    //
    , stakesPage(new Widget(_parent))
    , includeStakes(new CheckBox(_parent))
    , stakes(new TextField(_parent))
    , stakesAdvise(new AdviseWidget(_parent))
    //
    , loglinePage(new Widget(_parent))
    , loglineIcon(new IconsBigLabel(_parent))
    , loglineTitle(new H6Label(_parent))
    , logline(new Body1Label(_parent))
    //
    , backButton(new Button(_parent))
    , closeButton(new Button(_parent))
    , continueButton(new Button(_parent))
    , doneButton(new Button(_parent))
    , buttonsLayout(new QHBoxLayout)
{
    content->setAnimationType(StackWidget::AnimationType::Slide);
    characterGender->setModel(characterGenderModel);
    theme->hide();
    mprEvent->hide();
    afterMprEvent->hide();
    worldSpecialRules->hide();
    stakes->hide();
    loglineIcon->setIcon(u8"\U000F012C");
    loglineIcon->setDecorationVisible(true);
    loglineTitle->setAlignment(Qt::AlignCenter);
    logline->setAlignment(Qt::AlignCenter);
    backButton->hide();
    doneButton->hide();

    {
        auto characterPageLayout = new QGridLayout;
        characterPageLayout->setContentsMargins({});
        characterPageLayout->setSpacing(0);
        characterPageLayout->addWidget(characterInfo, 0, 0);
        characterPageLayout->addWidget(characterGender, 1, 0);
        characterPageLayout->addWidget(characterAdvise, 0, 1, 3, 1);
        characterPageLayout->setColumnStretch(0, 3);
        characterPageLayout->setColumnStretch(1, 2);
        characterPageLayout->setRowStretch(2, 1);
        characterPage->setLayout(characterPageLayout);
        content->setCurrentWidget(characterPage);
    }

    {
        auto majorEventPageLayout = new QGridLayout;
        majorEventPageLayout->setContentsMargins({});
        majorEventPageLayout->setSpacing(0);
        majorEventPageLayout->addWidget(majorEvent, 0, 0);
        majorEventPageLayout->addWidget(majorEventIncludesMainChanager, 1, 0);
        majorEventPageLayout->addWidget(majorEventAdvise, 0, 1, 3, 1);
        majorEventPageLayout->setColumnStretch(0, 3);
        majorEventPageLayout->setColumnStretch(1, 2);
        majorEventPageLayout->setRowStretch(1, 1);
        majorEventPageLayout->setRowStretch(2, 1);
        majorEventPage->setLayout(majorEventPageLayout);
        content->addWidget(majorEventPage);
    }

    {
        auto themePageLayout = new QGridLayout;
        themePageLayout->setContentsMargins({});
        themePageLayout->setSpacing(0);
        themePageLayout->addWidget(includeTheme, 0, 0);
        themePageLayout->addWidget(theme, 1, 0);
        themePageLayout->addWidget(themeAdvise, 0, 1, 3, 1);
        themePageLayout->setColumnStretch(0, 3);
        themePageLayout->setColumnStretch(1, 2);
        themePageLayout->setRowStretch(2, 1);
        themePage->setLayout(themePageLayout);
        content->addWidget(themePage);
    }

    {
        auto storyGoalLayout = new QGridLayout;
        storyGoalLayout->setContentsMargins({});
        storyGoalLayout->setSpacing(0);
        storyGoalLayout->addWidget(storyGoal, 0, 0);
        storyGoalLayout->addWidget(storyGoalAdvise, 0, 1, 2, 1);
        storyGoalLayout->setColumnStretch(0, 3);
        storyGoalLayout->setColumnStretch(1, 2);
        storyGoalLayout->setRowStretch(1, 1);
        storyGoalPage->setLayout(storyGoalLayout);
        content->addWidget(storyGoalPage);
    }

    {
        auto mprPageLayout = new QGridLayout;
        mprPageLayout->setContentsMargins({});
        mprPageLayout->setSpacing(0);
        mprPageLayout->addWidget(includeMpr, 0, 0);
        mprPageLayout->addWidget(mprEvent, 1, 0);
        mprPageLayout->addWidget(afterMprEvent, 2, 0);
        mprPageLayout->addWidget(mprAdvise, 0, 1, 4, 1);
        mprPageLayout->setColumnStretch(0, 3);
        mprPageLayout->setColumnStretch(1, 2);
        mprPageLayout->setRowStretch(3, 1);
        mprPage->setLayout(mprPageLayout);
        content->addWidget(mprPage);
    }

    {
        auto worldPageLayout = new QGridLayout;
        worldPageLayout->setContentsMargins({});
        worldPageLayout->setSpacing(0);
        worldPageLayout->addWidget(worldHasSpecialRules, 0, 0);
        worldPageLayout->addWidget(worldSpecialRules, 1, 0);
        worldPageLayout->addWidget(worldAdvise, 0, 1, 3, 1);
        worldPageLayout->setColumnStretch(0, 3);
        worldPageLayout->setColumnStretch(1, 2);
        worldPageLayout->setRowStretch(2, 1);
        worldPage->setLayout(worldPageLayout);
        content->addWidget(worldPage);
    }

    {
        auto stakesPageLayout = new QGridLayout;
        stakesPageLayout->setContentsMargins({});
        stakesPageLayout->setSpacing(0);
        stakesPageLayout->addWidget(includeStakes, 0, 0);
        stakesPageLayout->addWidget(stakes, 1, 0);
        stakesPageLayout->addWidget(stakesAdvise, 0, 1, 3, 1);
        stakesPageLayout->setColumnStretch(0, 3);
        stakesPageLayout->setColumnStretch(1, 2);
        stakesPageLayout->setRowStretch(2, 1);
        stakesPage->setLayout(stakesPageLayout);
        content->addWidget(stakesPage);
    }

    {
        auto loglinePageLayout = new QGridLayout;
        int row = 0;
        loglinePageLayout->setRowStretch(row++, 2);
        loglinePageLayout->addWidget(loglineIcon, row++, 0, Qt::AlignHCenter);
        loglinePageLayout->addWidget(loglineTitle, row++, 0);
        loglinePageLayout->addWidget(logline, row++, 0);
        loglinePageLayout->setRowStretch(row, 3);
        loglinePage->setLayout(loglinePageLayout);
        content->addWidget(loglinePage);
    }

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(backButton, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(closeButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(continueButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(doneButton, 0, Qt::AlignVCenter);
}

QWidget* LoglineGeneratorDialog::Implementation::page(Page _page) const
{
    switch (_page) {
    case Character: {
        return characterPage;
    }

    case MajorEvent: {
        return majorEventPage;
    }

    case Theme: {
        return themePage;
    }

    case StoryGoal: {
        return storyGoalPage;
    }

    case Mpr: {
        return mprPage;
    }

    case World: {
        return worldPage;
    }

    case Stakes: {
        return stakesPage;
    }

    case Logline: {
        return loglinePage;
    }
    }
}

QWidget* LoglineGeneratorDialog::Implementation::pageFocus(Page _page) const
{
    switch (_page) {
    case Character: {
        return characterInfo;
    }

    case MajorEvent: {
        return majorEvent;
    }

    case Theme: {
        return includeTheme;
    }

    case StoryGoal: {
        return storyGoal;
    }

    case Mpr: {
        return includeMpr;
    }

    case World: {
        return worldHasSpecialRules;
    }

    case Stakes: {
        return includeStakes;
    }

    case Logline: {
        return loglinePage;
    }
    }
}

bool LoglineGeneratorDialog::Implementation::checkCurrentPage()
{
    switch (currentPage) {
    case Character: {
        if (characterInfo->text().isEmpty()) {
            characterInfo->setError(tr("No way! Main Character description is a must."));
            characterInfo->setFocus();
            return false;
        }

        return true;
    }

    case Theme: {
        if (includeTheme->isChecked() && theme->text().isEmpty()) {
            theme->setError(tr("Since you decide to include Theme, please write it out."));
            theme->setFocus();
            return false;
        }

        return true;
    }

    case StoryGoal: {
        if (storyGoal->text().isEmpty()) {
            storyGoal->setError(tr("Come on! No goal, no story! Please fill it."));
            storyGoal->setFocus();
            return false;
        }

        return true;
    }

    case Mpr: {
        if (includeMpr->isChecked()) {
            if (mprEvent->text().isEmpty()) {
                mprEvent->setError(tr("Since you decide to include MPR, please write it out."));
                mprEvent->setFocus();
                return false;
            }
            if (afterMprEvent->text().isEmpty()) {
                afterMprEvent->setError(
                    tr("Please write out what new approach Main Character should take."));
                afterMprEvent->setFocus();
                return false;
            }
        }

        return true;
    }

    case World: {
        if (worldHasSpecialRules->isChecked() && worldSpecialRules->text().isEmpty()) {
            worldSpecialRules->setError(tr("Please fill information about special rules?"));
            worldSpecialRules->setFocus();
            return false;
        }

        return true;
    }

    case Stakes: {
        if (includeStakes->isChecked() && stakes->text().isEmpty()) {
            stakes->setError(
                tr("No pain, no game! So, what fears of the Main Character comes true?"));
            stakes->setFocus();
            return false;
        }

        return true;
    }

    //
    // В самом базовом случае можно ничего не заполнять
    //
    default: {
        return true;
    }
    }
}

QString LoglineGeneratorDialog::Implementation::buildLogline() const
{
    QString logline;
    const auto characterPronoun = characterGender->currentIndex().row() == 0
        ? tr("he")
        : (characterGender->currentIndex().row() == 1 ? tr("she") : tr("it"));

    // Q1
    if (!majorEvent->text().isEmpty()) {
        logline += QString("%1 %2, ").arg(tr("When"), majorEvent->text());
    }
    // Q8
    if (worldHasSpecialRules->isChecked()) {
        logline += QString("%1 %2, ").arg(tr("in a world where"), worldSpecialRules->text());
    }
    // Q1 && Q8
    if (!majorEvent->text().isEmpty() && majorEventIncludesMainChanager->isChecked()) {
        logline += characterPronoun;
    }
    // !Q1 || !Q8
    else {
        logline += characterInfo->text();
    }
    //
    logline += QString(" %1 ").arg(tr("must"));
    // !Q7 && !Q5
    if (!includeMpr->isChecked() && !includeTheme->isChecked()) {
        //
        // Тут по алгоритму дублируется must
        //
        //        logline += QString("%1 ").arg(tr("must"));
        // Q5
        if (includeTheme->isChecked()) {
            logline += QString("%1 %2 %3 %4 ")
                           .arg(theme->text(), tr("in order to"), characterPronoun, tr("can"));
        }
        //
        // Тут странно, т.к. mpr не задан (переделал на цель истории)
        //
        logline += QString("%1 ").arg(storyGoal->text());
    } else {
        // !Q7 && Q5
        if (!includeMpr->isChecked() && includeTheme->isChecked()) {
            logline += QString("%1 %2 %3 %4 ")
                           .arg(theme->text(), tr("in order to"), characterPronoun, tr("can"));
        }
        //
        logline += QString("%1 ").arg(storyGoal->text());
        // Q7
        if (includeMpr->isChecked()) {
            logline = logline.trimmed(); // убираем пробел в конце
            logline
                += QString("; %1 %2 %3, %4 %5 ")
                       .arg(tr("but"), tr("when"), mprEvent->text(), characterPronoun, tr("must"));
            // Q5
            if (includeTheme->isChecked()) {
                logline += QString("%1 %2 %3 %4 ")
                               .arg(theme->text(), tr("in order to"), characterPronoun, tr("can"));
            }
            logline += QString("%1 ").arg(afterMprEvent->text());
        }
    }
    // Q9
    if (includeStakes->isChecked()) {
        logline += QString("%1 %2 ").arg(tr("before"), stakes->text());
    }

    //
    // Наводим красоту
    //
    logline = logline.remove(".").simplified().trimmed() + ".";
    if (logline.length() > 0 && logline.at(0).toUpper() != logline.at(0)) {
        logline[0] = TextHelper::smartToUpper(logline.at(0));
    }

    return logline;
}


// ****


LoglineGeneratorDialog::LoglineGeneratorDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->continueButton);
    setRejectButton(d->backButton);

    new Shadow(Qt::TopEdge, d->content);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->progressBar, row++, 0);
    contentsLayout()->addWidget(d->content, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);


    connect(d->characterInfo, &TextField::textChanged, d->characterInfo, &TextField::clearError);
    connect(d->includeTheme, &CheckBox::checkedChanged, d->theme, &TextField::setVisible);
    connect(d->theme, &TextField::textChanged, d->theme, &TextField::clearError);
    connect(d->storyGoal, &TextField::textChanged, d->storyGoal, &TextField::clearError);
    connect(d->includeMpr, &CheckBox::checkedChanged, d->mprEvent, &TextField::setVisible);
    connect(d->includeMpr, &CheckBox::checkedChanged, d->afterMprEvent, &TextField::setVisible);
    connect(d->mprEvent, &TextField::textChanged, d->mprEvent, &TextField::clearError);
    connect(d->afterMprEvent, &TextField::textChanged, d->afterMprEvent, &TextField::clearError);
    connect(d->worldHasSpecialRules, &CheckBox::checkedChanged, d->worldSpecialRules,
            &TextField::setVisible);
    connect(d->worldSpecialRules, &TextField::textChanged, d->worldSpecialRules,
            &TextField::clearError);
    connect(d->includeStakes, &CheckBox::checkedChanged, d->stakes, &TextField::setVisible);
    connect(d->stakes, &TextField::textChanged, d->stakes, &TextField::clearError);
    connect(d->continueButton, &Button::clicked, this, [this] {
        if (d->currentPage == Logline) {
            return;
        }

        const auto isCurrentPageOk = d->checkCurrentPage();
        if (!isCurrentPageOk) {
            return;
        }
        d->currentPage = static_cast<Page>(d->currentPage + 1);

        const bool isOnLastPage = d->currentPage == Logline;
        d->doneButton->setVisible(isOnLastPage);
        d->continueButton->setVisible(!isOnLastPage);
        d->backButton->setVisible(true);

        if (isOnLastPage) {
            d->logline->setText(d->buildLogline());
        }

        d->progressBar->setProgress(std::min(1.0, d->progressBar->progress() + 1 / 7.0));
        d->progressBar->setFocus();
        d->content->setCurrentWidget(d->page(d->currentPage));
        QTimer::singleShot(300, this, [this] { d->pageFocus(d->currentPage)->setFocus(); });
    });
    connect(d->backButton, &Button::clicked, this, [this] {
        if (d->currentPage == Character) {
            return;
        }

        d->currentPage = static_cast<Page>(d->currentPage - 1);

        const bool isOnFirstPage = d->currentPage == Character;
        d->continueButton->setVisible(true);
        d->doneButton->setVisible(false);
        d->backButton->setVisible(!isOnFirstPage);

        d->progressBar->setProgress(std::max(0.0, d->progressBar->progress() - 1 / 7.0));
        d->progressBar->setFocus();
        d->content->setCurrentWidget(d->page(d->currentPage));
        QTimer::singleShot(300, this, [this] { d->pageFocus(d->currentPage)->setFocus(); });
    });
    connect(d->closeButton, &Button::clicked, this, &LoglineGeneratorDialog::hideDialog);
    connect(d->doneButton, &Button::clicked, this, &LoglineGeneratorDialog::donePressed);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

LoglineGeneratorDialog::~LoglineGeneratorDialog() = default;

QString LoglineGeneratorDialog::logline() const
{
    return d->logline->text();
}

QWidget* LoglineGeneratorDialog::focusedWidgetAfterShow() const
{
    return d->characterInfo;
}

QWidget* LoglineGeneratorDialog::lastFocusableWidget() const
{
    return d->closeButton;
}

void LoglineGeneratorDialog::updateTranslations()
{
    setTitle(tr("Logline generator"));

    d->characterInfo->setLabel(tr("Describe your Main Character"));
    d->characterGender->setLabel(tr("What is the gender of your Main Character"));
    d->characterGenderModel->setStringList({ tr("Male"), tr("Female"), tr("Other") });
    d->characterGender->setCurrentText(tr("Other"));
    d->characterAdvise->setText(
        tr("Include psychological strength/weakness (optional) + function/job (mandatory).\n\nNo "
           "name, please. Just who they are and what they do, e.g. \"a lonesome cowboy\""
           " or \"an arrogant publicist\"."));
    //
    d->majorEvent->setLabel(tr("What is the Major Event that kicks off the story?"));
    d->majorEventIncludesMainChanager->setText(
        tr("Does the Major Event include the Main Character? \nIf \"yes\" then the Main Character "
           "input becomes he/she/it."));
    d->majorEventAdvise->setText(
        QString("%1\n\n%2\n\n%3")
            .arg(tr("This is the Inciting Incident, or Call To Adventure. It mostly happens within "
                    "the first 10 to 15 minutes of the story."),
                 tr("Full sentence, e.g. \"A shark kills a swimmer.\" or \"Her best friend shoots "
                    "the man who tried to rape her.\", \"Snakes are found on an airliner "
                    "in-flight.\""),
                 tr("Although this is optional, I strongly recommend you include the Major Event "
                    "if you know what it is. Some call this the Catalyst, Inciting Incident or "
                    "Call To Adventure. It usually happens in the first 10-20 min of the film.")));
    //
    d->includeTheme->setText(tr("Include a Theme or Character Arc?"));
    d->theme->setLabel(tr("What does the Main Character have to learn/overcome?"));
    d->themeAdvise->setText(
        QString("%1\n\n%2")
            .arg(tr("Theme is essential for the story, but optional for the logline."),
                 tr("Start your answer with \"To\", E.g. \"to become less selfish\", "
                    "\"to find purpose\", \"to re-establish her self-esteem\"")));
    //
    d->storyGoal->setLabel(tr("What is the story's main Action/Goal?"));
    d->storyGoalAdvise->setText(
        tr("This is what the character pursues for most of the story. Start your answer with "
           "\"To\", E.g. \"to find his son and return him home\"."));
    //
    d->includeMpr->setText(tr("Include an Mid Point Reversal Event?"));
    d->mprEvent->setLabel(tr("What is the Mid Point Reversal Event?"));
    d->afterMprEvent->setLabel(tr("What does the Main Character do/pursue after the Event?"));
    d->mprAdvise->setText(QString("%1\n\n%2")
                              .arg(tr("This Event makes the character change their behaviour "
                                      "usually for the better in the 2nd half of the story."),
                                   tr("This can be a change of approach, or a new goal entirely, "
                                      "E.g. \"to venture into the ocean to kill the shark\".")));
    //
    d->worldHasSpecialRules->setText(tr("Is this a special world, with special rules?"));
    d->worldSpecialRules->setLabel(tr("How is the world of your story different from reality?"));
    d->worldAdvise->setText(
        tr("Start your answer with \"It is\", e.g. \"it is a post-apocalyptical world\", or \"it "
           "is a world where people communicate through text bubbles\"."));
    //
    d->includeStakes->setText(tr("Include a Deadline/Stakes?"));
    d->stakes->setLabel(tr("What happens if the main character fails?"));
    d->stakesAdvise->setText(tr("Full sentence, e.g. \"The police catches the girls\", \"The "
                                "terrorists\", \"The bomb explodes\"."));
    //
    d->loglineTitle->setText(tr("Your logline is cooked!"));

    d->continueButton->setText(tr("Continue"));
    d->backButton->setText(tr("Back"));
    d->closeButton->setText(tr("Close"));
    d->doneButton->setText(tr("Done"));
}

void LoglineGeneratorDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(Ui::DesignSystem::layout().px(800));

    d->progressBar->setBackgroundColor(Ui::DesignSystem::color().background());
    d->content->setBackgroundColor(Ui::DesignSystem::color().background());

    for (auto page : {
             d->characterPage,
             d->majorEventPage,
             d->themePage,
             d->storyGoalPage,
             d->mprPage,
             d->worldPage,
             d->stakesPage,
             d->loglinePage,
         }) {
        page->setBackgroundColor(Ui::DesignSystem::color().background());
        page->layout()->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);
    }

    for (auto textField : std::vector<TextField*>{
             d->characterInfo,
             d->characterGender,
             d->majorEvent,
             d->theme,
             d->storyGoal,
             d->mprEvent,
             d->afterMprEvent,
             d->worldSpecialRules,
             d->stakes,
         }) {
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto textField : std::vector<TextField*>{
             d->characterInfo,
             d->mprEvent,
         }) {
        textField->setCustomMargins({ Ui::DesignSystem::layout().px24(), 0,
                                      Ui::DesignSystem::layout().px24(),
                                      Ui::DesignSystem::layout().px24() });
    }
    d->characterGender->setPopupBackgroundColor(Ui::DesignSystem::color().background());

    for (auto checkBox : { d->majorEventIncludesMainChanager, d->includeTheme, d->includeMpr,
                           d->worldHasSpecialRules, d->includeStakes }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto label : std::vector<Widget*>{
             d->loglineIcon,
             d->loglineTitle,
             d->logline,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
        label->setContentsMargins({});
    }
    d->loglineIcon->setTextColor(Ui::DesignSystem::color().secondary());
    d->loglineTitle->setContentsMargins(Ui::DesignSystem::layout().px24(),
                                        Ui::DesignSystem::layout().px24(),
                                        Ui::DesignSystem::layout().px24(), 0);
    d->logline->setContentsMargins(Ui::DesignSystem::layout().px(160),
                                   Ui::DesignSystem::layout().px24(),
                                   Ui::DesignSystem::layout().px(160), 0);

    d->continueButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->continueButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->doneButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->doneButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->closeButton->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->closeButton->setTextColor(Ui::DesignSystem::color().onBackground());
    d->backButton->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->backButton->setTextColor(Ui::DesignSystem::color().onBackground());

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
