#include "projects_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>

#include <utils/helpers/color_helper.h>

#include <QVBoxLayout>


namespace Ui
{

/**

https://screencraft.org/2013/01/23/50-great-screenwriting-quotes/
https://www.brainyquote.com/topics/screenwriter-quotes
https://scriptlarva.wordpress.com/inspiration-for-writers/
https://www.writersdigest.com/editor-blogs/there-are-no-rules/72-of-the-best-quotes-about-writing
https://gointothestory.blcklst.com/free-screenwriting-resource-writers-on-writing-quotes-e2f6f0d1f710
https://www.la-screenwriter.com/screenwriting-quotes/

 */

class ProjectsNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Цитата известного автора
     */
    struct Quote {
        QString text;
        QString author;
    };

    /**
     * @brief Сформировать цитату
     */
    Quote generateQuote() const;


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

ProjectsNavigator::Implementation::Quote ProjectsNavigator::Implementation::generateQuote() const
{
    return { "Let the world burn through you. Throw the prism light, white hot, on paper.",
             "Ray Bradbury" };
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
    const auto quote = d->generateQuote();
    d->quoteLabel->setText(quote.text);
    d->quoteAuthorLabel->setText(quote.author);

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
