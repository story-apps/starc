#include "logline_generator_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/progress_bar/progress_bar.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QDesktopServices>
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
class HintWidget : public Widget
{
public:
    explicit HintWidget(QWidget* _parent = nullptr)
        : Widget(_parent)
        , m_icon(new IconsMidLabel(this))
        , m_title(new H6Label(this))
        , m_text(new Body2Label(this))
        , m_readMore(new Body2LinkLabel(this))
    {
        m_icon->setIcon(u8"\U000F0335");

        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(m_icon);
        titleLayout->addWidget(m_title, 1);

        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(titleLayout);
        layout->addWidget(m_text);
        layout->addWidget(m_readMore, 0, Qt::AlignLeft);
        layout->addStretch();
        setLayout(layout);
    }

    void setText(const QString& _text)
    {
        m_text->setText(_text);
    }

    void setLink(const QString _link)
    {
        m_readMore->setLink(_link);
    }

protected:
    void updateTranslations() override
    {
        m_title->setText(LoglineGeneratorDialog::tr("Hint"));
        m_readMore->setText(LoglineGeneratorDialog::tr("Read more"));
    }

    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override
    {
        Widget::designSystemChangeEvent(_event);
        setFixedHeight(DesignSystem::layout().px(300));

        for (auto widget : std::vector<Widget*>{
                 this,
                 m_icon,
                 m_title,
                 m_text,
                 m_readMore,
             }) {
            widget->setBackgroundColor(DesignSystem::color().background());
            widget->setTextColor(DesignSystem::color().onBackground());
        }

        m_icon->setContentsMargins(0, 0, DesignSystem::layout().px8(), 0);
        m_title->setContentsMargins(0, 0, 0, DesignSystem::layout().px12());
        m_text->setContentsMargins(DesignSystem::layout().px4(), 0, DesignSystem::layout().px24(),
                                   0);
        m_readMore->setContentsMargins(DesignSystem::layout().px4(), DesignSystem::layout().px12(),
                                       DesignSystem::layout().px24(), 0);
        m_readMore->setTextColor(DesignSystem::color().accent());
    }

private:
    IconsMidLabel* m_icon = nullptr;
    H6Label* m_title = nullptr;
    Body2Label* m_text = nullptr;
    Body2LinkLabel* m_readMore = nullptr;
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
    HintWidget* characterHint = nullptr;

    Widget* majorEventPage = nullptr;
    TextField* majorEvent = nullptr;
    CheckBox* majorEventIncludesMainChanager = nullptr;
    HintWidget* majorEventHint = nullptr;

    Widget* themePage = nullptr;
    CheckBox* includeTheme = nullptr;
    TextField* theme = nullptr;
    HintWidget* themeHint = nullptr;

    Widget* storyGoalPage = nullptr;
    TextField* storyGoal = nullptr;
    HintWidget* storyGoalHint = nullptr;

    Widget* mprPage = nullptr;
    CheckBox* includeMpr = nullptr;
    TextField* mprEvent = nullptr;
    TextField* afterMprEvent = nullptr;
    HintWidget* mprHint = nullptr;

    Widget* worldPage = nullptr;
    CheckBox* worldHasSpecialRules = nullptr;
    TextField* worldSpecialRules = nullptr;
    HintWidget* worldHint = nullptr;

    Widget* stakesPage = nullptr;
    CheckBox* includeStakes = nullptr;
    TextField* stakes = nullptr;
    HintWidget* stakesHint = nullptr;

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
    , characterHint(new HintWidget(_parent))
    //
    , majorEventPage(new Widget(_parent))
    , majorEvent(new TextField(_parent))
    , majorEventIncludesMainChanager(new CheckBox(_parent))
    , majorEventHint(new HintWidget(_parent))
    //
    , themePage(new Widget(_parent))
    , includeTheme(new CheckBox(_parent))
    , theme(new TextField(_parent))
    , themeHint(new HintWidget(_parent))
    //
    , storyGoalPage(new Widget(_parent))
    , storyGoal(new TextField(_parent))
    , storyGoalHint(new HintWidget(_parent))
    //
    , mprPage(new Widget(_parent))
    , includeMpr(new CheckBox(_parent))
    , mprEvent(new TextField(_parent))
    , afterMprEvent(new TextField(_parent))
    , mprHint(new HintWidget(_parent))
    //
    , worldPage(new Widget(_parent))
    , worldHasSpecialRules(new CheckBox(_parent))
    , worldSpecialRules(new TextField(_parent))
    , worldHint(new HintWidget(_parent))
    //
    , stakesPage(new Widget(_parent))
    , includeStakes(new CheckBox(_parent))
    , stakes(new TextField(_parent))
    , stakesHint(new HintWidget(_parent))
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
        characterPageLayout->addWidget(characterHint, 0, 1, 3, 1);
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
        majorEventPageLayout->addWidget(majorEventHint, 0, 1, 3, 1);
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
        themePageLayout->addWidget(themeHint, 0, 1, 3, 1);
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
        storyGoalLayout->addWidget(storyGoalHint, 0, 1, 2, 1);
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
        mprPageLayout->addWidget(mprHint, 0, 1, 4, 1);
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
        worldPageLayout->addWidget(worldHint, 0, 1, 3, 1);
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
        stakesPageLayout->addWidget(stakesHint, 0, 1, 3, 1);
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

    UiHelper::initSpellingFor({
        characterInfo,
        majorEvent,
        theme,
        storyGoal,
        mprEvent,
        afterMprEvent,
        worldSpecialRules,
        stakes,
    });
    UiHelper::initOptionsFor({
        characterInfo,
        majorEvent,
        theme,
        storyGoal,
        mprEvent,
        afterMprEvent,
        worldSpecialRules,
        stakes,
    });

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
        if (includeTheme->isChecked()) {
            return theme;
        } else {
            return includeTheme;
        }
    }

    case StoryGoal: {
        return storyGoal;
    }

    case Mpr: {
        if (includeMpr->isChecked()) {
            return mprEvent;
        } else {
            return includeMpr;
        }
    }

    case World: {
        if (worldHasSpecialRules->isChecked()) {
            return worldSpecialRules;
        } else {
            return worldHasSpecialRules;
        }
    }

    case Stakes: {
        if (includeStakes->isChecked()) {
            return stakes;
        } else {
            return includeStakes;
        }
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
            characterInfo->setError(
                tr("Every successful screen story has a central character. Describe it!"));
            characterInfo->setFocus();
            return false;
        }

        return true;
    }

    case Theme: {
        if (includeTheme->isChecked() && theme->text().isEmpty()) {
            theme->setError(tr("Untick if you don't want to include theme."));
            theme->setFocus();
            return false;
        }

        return true;
    }

    case StoryGoal: {
        if (storyGoal->text().isEmpty()) {
            storyGoal->setError(tr("Come on! No goal, no story! Complete it, then move on."));
            storyGoal->setFocus();
            return false;
        }

        return true;
    }

    case Mpr: {
        if (includeMpr->isChecked()) {
            if (mprEvent->text().isEmpty()) {
                mprEvent->setError(tr("Untick if you don't want to include MPR."));
                mprEvent->setFocus();
                return false;
            }
            if (afterMprEvent->text().isEmpty()) {
                afterMprEvent->setError(tr("Untick if you don't want to include MPR."));
                afterMprEvent->setFocus();
                return false;
            }
        }

        return true;
    }

    case World: {
        if (worldHasSpecialRules->isChecked() && worldSpecialRules->text().isEmpty()) {
            worldSpecialRules->setError(tr("Untick if you don't want to include special rules."));
            worldSpecialRules->setFocus();
            return false;
        }

        return true;
    }

    case Stakes: {
        if (includeStakes->isChecked() && stakes->text().isEmpty()) {
            stakes->setError(tr("Untick if you don't want to include stakes."));
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
    auto text = [](TextField* _textField) {
        auto text = _textField->text().trimmed();
        if (text.startsWith("to ", Qt::CaseInsensitive)) {
            text = text.mid(3);
        }
        if (text.length() > 0) {
            text[0] = text[0].toLower();
        }
        return text;
    };

    QString logline;
    const auto characterPronoun = characterGender->currentIndex().row() == 0
        ? tr("he")
        : (characterGender->currentIndex().row() == 1 ? tr("she") : tr("they"));

    // Q1
    if (!majorEvent->text().isEmpty()) {
        logline += QString("%1 %2, ").arg(tr("When"), text(majorEvent));
    }
    // Q8
    if (worldHasSpecialRules->isChecked()) {
        auto worldText = text(worldSpecialRules);
        if (!worldText.contains(" world where", Qt::CaseInsensitive)) {
            logline = logline.remove("it is", Qt::CaseInsensitive);
        }
        logline += QString("%1 %2, ").arg(tr("in"), worldText);
    }
    // Q1 && Q8
    if (!majorEvent->text().isEmpty() && majorEventIncludesMainChanager->isChecked()) {
        logline += characterPronoun;
    }
    // !Q1 || !Q8
    else {
        logline += text(characterInfo);
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
            logline += QString("%1 %2 ").arg(text(theme), tr("in order to"));
        }
        //
        // Тут странно, т.к. mpr не задан (переделал на цель истории)
        //
        logline += QString("%1 ").arg(text(storyGoal));
    } else {
        // !Q7 && Q5
        if (!includeMpr->isChecked() && includeTheme->isChecked()) {
            logline += QString("%1 %2 ").arg(text(theme), tr("in order to"));
        }
        //
        logline += QString("%1").arg(text(storyGoal));
        // Q7
        if (includeMpr->isChecked()) {
            logline = logline.trimmed(); // убираем пробел в конце
            logline
                += QString("; %1 %2 %3, %4 %5 ")
                       .arg(tr("but"), tr("when"), text(mprEvent), characterPronoun, tr("must"));
            // Q5
            if (includeTheme->isChecked()) {
                logline += QString("%1 %2 ").arg(text(theme), tr("in order to"));
            }
            logline += QString("%1 ").arg(text(afterMprEvent));
        } else {
            logline += " ";
        }
    }
    // Q9
    if (includeStakes->isChecked()) {
        logline += QString("%1 %2 ").arg(tr("before"), text(stakes));
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

    titleIcon()->setIcon(u8"\U000F02D7");
    titleIcon()->setCheckable(true);
    titleIcon()->show();

    new Shadow(Qt::TopEdge, d->content);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->progressBar, row++, 0);
    contentsLayout()->addWidget(d->content, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row, 0);
    contentsLayout()->setRowStretch(row, 1);


    auto setHintsVisibile = [this](bool _visible) {
        auto updateVisibility = [_visible](QWidget* _hint, QWidget* _page) {
            _hint->setMaximumWidth(_visible ? QWIDGETSIZE_MAX : 1);
            auto layout = static_cast<QGridLayout*>(_page->layout());
            layout->setColumnStretch(0, _visible ? 3 : 0);
            layout->setColumnStretch(1, _visible ? 2 : 0);
        };
        updateVisibility(d->characterHint, d->characterPage);
        updateVisibility(d->majorEventHint, d->majorEventPage);
        updateVisibility(d->themeHint, d->themePage);
        updateVisibility(d->storyGoalHint, d->storyGoalPage);
        updateVisibility(d->mprHint, d->mprPage);
        updateVisibility(d->worldHint, d->worldPage);
        updateVisibility(d->stakesHint, d->stakesPage);
    };
    connect(titleIcon(), &IconButton::checkedChanged, this, setHintsVisibile);
    connect(d->characterInfo, &TextField::textChanged, d->characterInfo, &TextField::clearError);
    connect(d->includeTheme, &CheckBox::checkedChanged, d->theme, &TextField::setVisible);
    connect(d->includeTheme, &CheckBox::checkedChanged, d->theme,
            qOverload<>(&TextField::setFocus));
    connect(d->theme, &TextField::textChanged, d->theme, &TextField::clearError);
    connect(d->storyGoal, &TextField::textChanged, d->storyGoal, &TextField::clearError);
    connect(d->includeMpr, &CheckBox::checkedChanged, d->mprEvent, &TextField::setVisible);
    connect(d->includeMpr, &CheckBox::checkedChanged, d->mprEvent,
            qOverload<>(&TextField::setFocus));
    connect(d->includeMpr, &CheckBox::checkedChanged, d->afterMprEvent, &TextField::setVisible);
    connect(d->mprEvent, &TextField::textChanged, d->mprEvent, &TextField::clearError);
    connect(d->afterMprEvent, &TextField::textChanged, d->afterMprEvent, &TextField::clearError);
    connect(d->worldHasSpecialRules, &CheckBox::checkedChanged, d->worldSpecialRules,
            &TextField::setVisible);
    connect(d->worldHasSpecialRules, &CheckBox::checkedChanged, d->worldSpecialRules,
            qOverload<>(&TextField::setFocus));
    connect(d->worldSpecialRules, &TextField::textChanged, d->worldSpecialRules,
            &TextField::clearError);
    connect(d->includeStakes, &CheckBox::checkedChanged, d->stakes, &TextField::setVisible);
    connect(d->includeStakes, &CheckBox::checkedChanged, d->stakes,
            qOverload<>(&TextField::setFocus));
    connect(d->stakes, &TextField::textChanged, d->stakes, &TextField::clearError);
    connect(d->continueButton, &Button::clicked, this, [this] {
        if (d->currentPage == Logline) {
            d->doneButton->click();
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

            //
            // Перенастроим кнопку диалога на шеринг
            //
            titleIcon()->disconnect(this);
            titleIcon()->setChecked(false);
            titleIcon()->setCheckable(false);
            titleIcon()->setIcon(u8"\U000F0544");
            titleIcon()->setToolTip(tr("Share your logline"));
            connect(titleIcon(), &IconButton::clicked, this, [this] {
                const auto message
                    = QString("%1 '%2'").arg(tr("Check out my new logline"), d->logline->text());
                QDesktopServices::openUrl(
                    QUrl(QString("https://twitter.com/intent/tweet?text=%1&via=starcapp_")
                             .arg(message)));
            });
        }

        d->progressBar->setProgress(std::min(1.0, d->progressBar->progress() + 1 / 7.0));
        d->progressBar->setFocus();
        d->content->setCurrentWidget(d->page(d->currentPage));
        QTimer::singleShot(300, this, [this] { d->pageFocus(d->currentPage)->setFocus(); });
    });
    connect(d->backButton, &Button::clicked, this, [this, setHintsVisibile] {
        if (d->currentPage == Character) {
            d->closeButton->click();
            return;
        }

        if (d->currentPage == Logline) {
            //
            // Перенастроим кнопку диалога на отображение справки
            //
            titleIcon()->disconnect(this);
            titleIcon()->setCheckable(true);
            titleIcon()->setChecked(d->stakesHint->width() > 1);
            titleIcon()->setIcon(u8"\U000F02D7");
            titleIcon()->setToolTip(tr("Show help"));
            connect(titleIcon(), &IconButton::checkedChanged, this, setHintsVisibile);
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


    setHintsVisibile(false);
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
    titleIcon()->setToolTip(tr("Show help"));

    d->characterInfo->setLabel(tr("Describe your main character"));
    d->characterInfo->setHelper(tr("E.g. 'A lonesome cowboy', or 'An arrogant publicist'."));
    d->characterGender->setLabel(tr("What is their gender?"));
    d->characterGenderModel->setStringList({ tr("Male"), tr("Female"), tr("Other") });
    d->characterGender->setCurrentText(tr("Other"));
    d->characterHint->setText(
        tr("Typically this includes the character's function or job, and some weakness "
           "and/or strength. Loglines only have character names if the names are already "
           "a part of popular culture."));
    //
    d->majorEvent->setLabel(tr("What event triggers the story's main action?"));
    d->majorEvent->setHelper(
        tr("E.g. 'A shark kills a swimmer' or 'her best friend shoots a man who tried to "
           "rape her'."));
    d->majorEventIncludesMainChanager->setText(
        tr("Did you include a description of the main character?"));
    d->majorEventHint->setText(QString("%1").arg(
        tr("Events are things that happen to the main character, and that motivates their "
           "actions. They're never anything the character does. Here we're looking for "
           "the story's main event or Inciting Incident / Call To Adventure. This "
           "typically happens in the first 10 - 20 minutes.")));
    //
    d->includeTheme->setText(tr("Include a Theme or Character Arc"));
    d->theme->setLabel(tr("What do they have to learn/overcome?"));
    d->theme->setHelper(tr("E.g. 'to become less selfish', or 'to find purpose in life'."));
    d->themeHint->setText(QString("%1").arg(
        tr("Even if theme is important in most stories, it is not critical to a basic "
           "logline. And often theme is established further down the development process. "
           "Leave it out for now, or try something and change it later.")));
    //
    d->storyGoal->setLabel(tr("What is the main Action / Goal?"));
    d->storyGoal->setHelper(
        tr("E.g. 'to close the beach', or 'to find her son and return him home'."));
    d->storyGoalHint->setText(QString("%1").arg(
        tr("The main action or goal is 'what the story is about' for the broad audience. "
           "It is that part of the story we can visualise when we hear the logline, and "
           "which (hopefully) makes us want to see the film or show episode. It is the "
           "objective the main character will pursue for most of the time.")));
    //
    d->includeMpr->setText(tr("Include a Mid Point Reversal (MPR)"));
    d->mprEvent->setLabel(tr("What is the MPR Event?"));
    d->mprEvent->setHelper(
        tr("E.g. 'the trip to Vegas is aborted', or 'his employer turns out to be his nemesis'."));
    d->afterMprEvent->setLabel(tr("What is the character's new Action / Goal?"));
    d->afterMprEvent->setHelper(tr("E.g. 'to venture into the ocean and kill the shark'"));
    d->mprHint->setText(QString("%1").arg(
        tr("This event often happens just when the goal is within reach. It flips the character's "
           "fortune, and any gains now seem lost. But it may also be the motivation for the "
           "character's change in goal or approach. After the MPR, they often change their "
           "behaviour and start doing 'the right thing'.")));
    //
    d->worldHasSpecialRules->setText(tr("Is the story set in a special, future or magical world?"));
    d->worldSpecialRules->setLabel(tr("How is your story world different from normal reality?"));
    d->worldSpecialRules->setHelper(
        tr("E.g. 'it is 1920's Paris', or 'it is a world where pigs fly'"));
    d->worldHint->setText(tr("Only add this if it is essential to understand the story. "));
    //
    d->includeStakes->setText(tr("Include a Deadline/Stakes?"));
    d->stakes->setLabel(tr("What happens if the main character fails?"));
    d->stakes->setHelper(tr("E.g. 'the police catches the girls', or 'the shark attacks again'."));
    d->stakesHint->setText(
        QString("%1\n\n%2")
            .arg(
                tr("To make the logline work, write this as a full sentence like in the examples."),
                tr("Often the stakes are clear from the Main Event. But in some cases, adding "
                   "negative stakes adds urgency to the logline, ultimately improving its "
                   "emotional impact.")));
    //
    auto link = [](const QString& _type) {
        return QString("https://starc.app/api/app/help/logline?type=%1&lang=%2")
            .arg(_type, QLocale::languageToString(QLocale().language()));
    };
    d->characterHint->setLink(link("character"));
    d->majorEventHint->setLink(link("major-event"));
    d->themeHint->setLink(link("theme"));
    d->storyGoalHint->setLink(link("story-goal"));
    d->mprHint->setLink(link("mpr"));
    d->worldHint->setLink(link("world"));
    d->stakesHint->setLink(link("stakes"));
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

    setContentFixedWidth(DesignSystem::layout().px(800));

    d->progressBar->setBackgroundColor(DesignSystem::color().background());
    d->content->setBackgroundColor(DesignSystem::color().background());

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
        page->setBackgroundColor(DesignSystem::color().background());
        page->layout()->setContentsMargins(0, DesignSystem::layout().px24(), 0, 0);
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
        textField->setTextColor(DesignSystem::color().onBackground());
        textField->setBackgroundColor(DesignSystem::color().onBackground());
    }
    for (auto textField : std::vector<TextField*>{
             d->characterInfo,
             d->mprEvent,
         }) {
        textField->setCustomMargins({ DesignSystem::layout().px24(), 0,
                                      DesignSystem::layout().px24(),
                                      DesignSystem::layout().px24() });
    }
    d->characterGender->setPopupBackgroundColor(DesignSystem::color().background());

    for (auto checkBox : { d->majorEventIncludesMainChanager, d->includeTheme, d->includeMpr,
                           d->worldHasSpecialRules, d->includeStakes }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto label : std::vector<Widget*>{
             d->loglineIcon,
             d->loglineTitle,
             d->logline,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins({});
    }
    d->loglineIcon->setTextColor(DesignSystem::color().accent());
    d->loglineTitle->setContentsMargins(DesignSystem::layout().px24(),
                                        DesignSystem::layout().px24(),
                                        DesignSystem::layout().px24(), 0);
    d->logline->setContentsMargins(DesignSystem::layout().px(160), DesignSystem::layout().px24(),
                                   DesignSystem::layout().px(160), 0);

    UiHelper::initColorsFor(d->continueButton, UiHelper::DialogAccept);
    UiHelper::initColorsFor(d->doneButton, UiHelper::DialogAccept);
    UiHelper::initColorsFor(d->closeButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->backButton, UiHelper::DialogDefault);

    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px12(), DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
