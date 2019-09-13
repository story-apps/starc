#include "projects_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/quotes_helper.h>

#include <QVBoxLayout>


namespace Ui
{

class ProjectsNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QuotesHelper::Quote quote;

    QVBoxLayout* layout = nullptr;
    H6Label* quoteLabel = nullptr;
    OverlineLabel* quoteAuthorLabel = nullptr;
    Button* createStoryButton = nullptr;
    Button* openStoryButton = nullptr;
    Button* helpButton = nullptr;
};

ProjectsNavigator::Implementation::Implementation(QWidget* _parent)
    : layout(new QVBoxLayout(_parent)),
      quoteLabel(new H6Label(_parent)),
      quoteAuthorLabel(new OverlineLabel(_parent)),
      createStoryButton(new Button(_parent)),
      openStoryButton(new Button(_parent)),
      helpButton(new Button(_parent))
{
    createStoryButton->setIcon("\uf415");
    openStoryButton->setIcon("\uf256");
    helpButton->setIcon("\uf2d7");
}


// ****


ProjectsNavigator::ProjectsNavigator(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    d->layout->setContentsMargins({});
    d->layout->setSpacing(0);
    d->layout->addWidget(d->quoteLabel);
    d->layout->addWidget(d->quoteAuthorLabel);
    d->layout->addWidget(d->createStoryButton);
    d->layout->addWidget(d->openStoryButton);
    d->layout->addWidget(d->helpButton);
    d->layout->addStretch();


    designSystemChangeEvent(nullptr);
}

void ProjectsNavigator::updateTranslations()
{
    d->quote = QuotesHelper::generateQuote(d->quote.index);
    d->quoteLabel->setText(d->quote.text);
    d->quoteAuthorLabel->setText(d->quote.author);

    d->createStoryButton->setText(tr("Create story"));
    d->openStoryButton->setText(tr("Open story"));
    d->helpButton->setText(tr("How to use application?"));
}

ProjectsNavigator::~ProjectsNavigator() = default;

void ProjectsNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setBackgroundColor(DesignSystem::color().primary());

    d->layout->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px12()).toMargins());
    const QMarginsF quoteMargins(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px24(),
                                 Ui::DesignSystem::layout().px16(), 0.0);
    d->quoteLabel->setContentsMargins(quoteMargins.toMargins());
    d->quoteLabel->setBackgroundColor(DesignSystem::color().primary());
    d->quoteLabel->setTextColor(DesignSystem::color().onPrimary());
    const QMarginsF quoteAuthorMargins(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px12(),
                                       Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px24() * 2);
    d->quoteAuthorLabel->setContentsMargins(quoteAuthorMargins.toMargins());
    d->quoteAuthorLabel->setBackgroundColor(DesignSystem::color().primary());
    d->quoteAuthorLabel->setTextColor(
                ColorHelper::colorBetween(DesignSystem::color().primary(),
                                          DesignSystem::color().onPrimary()));
    for (auto button : {d->createStoryButton, d->openStoryButton, d->helpButton}) {
        button->setBackgroundColor(DesignSystem::color().secondary());
        button->setTextColor(DesignSystem::color().secondary());
    }
}

} // namespace Ui
