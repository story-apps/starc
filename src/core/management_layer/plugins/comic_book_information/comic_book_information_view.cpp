#include "comic_book_information_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class ComicBookInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* screenplayInfo = nullptr;
    QGridLayout* screenplayInfoLayout = nullptr;
    TextField* screenplayName = nullptr;
    TextField* screenplayTagline = nullptr;
    TextField* screenplayLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* screenplayTextVisiblity = nullptr;
    CheckBox* screenplayStatisticsVisiblity = nullptr;
};

ComicBookInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , screenplayInfo(new Card(_parent))
    , screenplayInfoLayout(new QGridLayout)
    , screenplayName(new TextField(screenplayInfo))
    , screenplayTagline(new TextField(screenplayInfo))
    , screenplayLogline(new TextField(screenplayInfo))
    , titlePageVisiblity(new CheckBox(screenplayInfo))
    , synopsisVisiblity(new CheckBox(screenplayInfo))
    , screenplayTextVisiblity(new CheckBox(screenplayInfo))
    , screenplayStatisticsVisiblity(new CheckBox(screenplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    screenplayInfoLayout->setContentsMargins({});
    screenplayInfoLayout->setSpacing(0);
    int row = 0;
    screenplayInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    screenplayInfoLayout->addWidget(screenplayName, row++, 0);
    screenplayInfoLayout->addWidget(screenplayTagline, row++, 0);
    screenplayInfoLayout->addWidget(screenplayLogline, row++, 0);
    screenplayInfoLayout->addWidget(titlePageVisiblity, row++, 0);
    screenplayInfoLayout->addWidget(synopsisVisiblity, row++, 0);
    screenplayInfoLayout->addWidget(screenplayTextVisiblity, row++, 0);
    screenplayInfoLayout->addWidget(screenplayStatisticsVisiblity, row++, 0);
    screenplayInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    screenplayInfoLayout->setColumnStretch(0, 1);
    screenplayInfo->setLayoutReimpl(screenplayInfoLayout);

    screenplayName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    UiHelper::initSpellingFor({ screenplayTagline, screenplayLogline });
    screenplayLogline->setEnterMakesNewLine(true);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(screenplayInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ComicBookInformationView::ComicBookInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->screenplayName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->screenplayName->text()); });
    connect(d->screenplayTagline, &TextField::textChanged, this,
            [this] { emit taglineChanged(d->screenplayTagline->text()); });
    connect(d->screenplayLogline, &TextField::textChanged, this,
            [this] { emit loglineChanged(d->screenplayLogline->text()); });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::synopsisVisibleChanged);
    connect(d->screenplayTextVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::comicBookTextVisibleChanged);
    connect(d->screenplayStatisticsVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::comicBookStatisticsVisibleChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ComicBookInformationView::~ComicBookInformationView() = default;

void ComicBookInformationView::setName(const QString& _name)
{
    if (d->screenplayName->text() == _name) {
        return;
    }

    d->screenplayName->setText(_name);
}

void ComicBookInformationView::setTagline(const QString& _tagline)
{
    if (d->screenplayTagline->text() == _tagline) {
        return;
    }

    d->screenplayTagline->setText(_tagline);
}

void ComicBookInformationView::setLogline(const QString& _logline)
{
    if (d->screenplayLogline->text() == _logline) {
        return;
    }

    d->screenplayLogline->setText(_logline);
}

void ComicBookInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void ComicBookInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void ComicBookInformationView::setComicBookTextVisible(bool _visible)
{
    d->screenplayTextVisiblity->setChecked(_visible);
}

void ComicBookInformationView::setComicBookStatisticsVisible(bool _visible)
{
    d->screenplayStatisticsVisiblity->setChecked(_visible);
}

void ComicBookInformationView::updateTranslations()
{
    d->screenplayName->setLabel(tr("Comic book name"));
    d->screenplayTagline->setLabel(tr("Tagline"));
    d->screenplayLogline->setLabel(tr("Logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->screenplayTextVisiblity->setText(tr("Script"));
    d->screenplayStatisticsVisiblity->setText(tr("Statistics"));
}

void ComicBookInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->screenplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->screenplayName, d->screenplayTagline, d->screenplayLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->titlePageVisiblity, d->synopsisVisiblity, d->screenplayTextVisiblity,
                           d->screenplayStatisticsVisiblity }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->screenplayInfoLayout->setVerticalSpacing(
        static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->screenplayInfoLayout->setRowMinimumHeight(
        0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->screenplayInfoLayout->setRowMinimumHeight(
        d->screenplayInfoLayout->rowCount() - 1,
        static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
