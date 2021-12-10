#include "projects_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/quotes_helper.h>

#include <QVBoxLayout>


namespace Ui {

class ProjectsNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QuotesHelper::Quote quote;

    QVBoxLayout* layout = nullptr;
    H6Label* quoteLabel = nullptr;
    OverlineLabel* quoteAuthorLabel = nullptr;
    Button* createProjectButton = nullptr;
    Button* openProjectButton = nullptr;
    Button* helpButton = nullptr;
};

ProjectsNavigator::Implementation::Implementation(QWidget* _parent)
    : layout(new QVBoxLayout(_parent))
    , quoteLabel(new H6Label(_parent))
    , quoteAuthorLabel(new OverlineLabel(_parent))
    , createProjectButton(new Button(_parent))
    , openProjectButton(new Button(_parent))
    , helpButton(new Button(_parent))
{
    createProjectButton->setIcon(u8"\U000f0415");
    openProjectButton->setIcon(u8"\U000f0256");
    helpButton->setIcon(u8"\U000f02d7");

    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(quoteLabel);
    layout->addWidget(quoteAuthorLabel);
    layout->addWidget(createProjectButton);
    layout->addWidget(openProjectButton);
    layout->addWidget(helpButton);
    layout->addStretch();
}


// ****


ProjectsNavigator::ProjectsNavigator(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    connect(d->createProjectButton, &Button::clicked, this,
            &ProjectsNavigator::createProjectPressed);
    connect(d->openProjectButton, &Button::clicked, this, &ProjectsNavigator::openProjectPressed);
    connect(d->helpButton, &Button::clicked, this, &ProjectsNavigator::helpPressed);

    designSystemChangeEvent(nullptr);
}

void ProjectsNavigator::updateTranslations()
{
    d->quote = QuotesHelper::generateQuote(d->quote.index);
    d->quoteLabel->setText(d->quote.text);
    d->quoteAuthorLabel->setText(d->quote.author);

    d->createProjectButton->setText(tr("Create story"));
    d->openProjectButton->setText(tr("Open story"));
    d->helpButton->setText(tr("How to use application?"));
}

ProjectsNavigator::~ProjectsNavigator() = default;

void ProjectsNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setBackgroundColor(DesignSystem::color().primary());

    d->layout->setSpacing(Ui::DesignSystem::layout().px12());
    d->layout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
    const QMarginsF quoteMargins(Ui::DesignSystem::layout().px16(),
                                 Ui::DesignSystem::layout().px16(),
                                 Ui::DesignSystem::layout().px16(), 0.0);
    d->quoteLabel->setContentsMargins(quoteMargins.toMargins());
    d->quoteLabel->setBackgroundColor(DesignSystem::color().primary());
    d->quoteLabel->setTextColor(DesignSystem::color().onPrimary());
    const QMarginsF quoteAuthorMargins(Ui::DesignSystem::layout().px16(), 0,
                                       Ui::DesignSystem::layout().px16(),
                                       Ui::DesignSystem::layout().px24() * 2);
    d->quoteAuthorLabel->setContentsMargins(quoteAuthorMargins.toMargins());
    d->quoteAuthorLabel->setBackgroundColor(DesignSystem::color().primary());
    d->quoteAuthorLabel->setTextColor(ColorHelper::colorBetween(DesignSystem::color().primary(),
                                                                DesignSystem::color().onPrimary()));
    for (auto button : { d->createProjectButton, d->openProjectButton, d->helpButton }) {
        button->setBackgroundColor(DesignSystem::color().secondary());
        button->setTextColor(DesignSystem::color().secondary());
    }
}

} // namespace Ui
