#include "project_information_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/image/image_card.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class ProjectInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* projectInfo = nullptr;
    QGridLayout* projectInfoLayout = nullptr;
    TextField* projectName = nullptr;
    TextField* projectLogline = nullptr;

    ImageCard* projectCover = nullptr;
};

ProjectInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , projectInfo(new Card(_parent))
    , projectInfoLayout(new QGridLayout)
    , projectName(new TextField(projectInfo))
    , projectLogline(new TextField(projectInfo))
    , projectCover(new ImageCard(projectInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);
    projectCover->setDecorationIcon(u8"\U000F02E9");

    projectInfoLayout->setContentsMargins({});
    projectInfoLayout->setSpacing(0);
    int row = 0;
    projectInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку над названием
    projectInfoLayout->addWidget(projectName, row++, 0);
    projectInfoLayout->addWidget(projectLogline, row++, 0);
    projectInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку под логлайном
    projectInfo->setLayoutReimpl(projectInfoLayout);

    projectName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    projectLogline->setEnterMakesNewLine(true);
    UiHelper::initSpellingFor(projectLogline);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    auto layout = new QGridLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(projectInfo, 0, 0);
    layout->addWidget(projectCover, 0, 1, 2, 1, Qt::AlignTop);
    layout->setRowStretch(1, 1);
    contentWidget->setLayout(layout);
}


// ****


ProjectInformationView::ProjectInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->projectName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->projectName->text()); });
    connect(d->projectLogline, &TextField::textChanged, this,
            [this] { emit loglineChanged(d->projectLogline->text()); });
    connect(d->projectCover, &ImageCard::imageChanged, this, &ProjectInformationView::coverChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ProjectInformationView::~ProjectInformationView() = default;

QWidget* ProjectInformationView::asQWidget()
{
    return this;
}

void ProjectInformationView::setName(const QString& _name)
{
    if (d->projectName->text() == _name) {
        return;
    }

    d->projectName->setText(_name);
}

void ProjectInformationView::setLogline(const QString& _logline)
{
    if (d->projectLogline->text() == _logline) {
        return;
    }

    d->projectLogline->setText(_logline);
}

void ProjectInformationView::setCover(const QPixmap& _cover)
{
    d->projectCover->setImage(_cover);
}

void ProjectInformationView::updateTranslations()
{
    d->projectName->setLabel(tr("Project name"));
    d->projectLogline->setLabel(tr("Short description"));
    d->projectLogline->setHelper(tr("Tagline, logline or something similar"));
    d->projectCover->setSupportingText(tr("Add cover +"), tr("Change cover..."),
                                       tr("Do you want to reset the story's cover?"));
    d->projectCover->setImageCroppingText(tr("Select an area for the cover"));
}

void ProjectInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->projectInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->projectName, d->projectLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->projectInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->projectInfoLayout->setRowMinimumHeight(0,
                                              static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->projectInfoLayout->setRowMinimumHeight(3,
                                              static_cast<int>(Ui::DesignSystem::layout().px16()));

    d->projectCover->setBackgroundColor(Ui::DesignSystem::color().background());
    d->projectCover->setTextColor(Ui::DesignSystem::color().onBackground());
    d->projectCover->setFixedSize((QSizeF(336, 510) * Ui::DesignSystem::scaleFactor()).toSize());
}

} // namespace Ui
