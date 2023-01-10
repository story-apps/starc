#include "project_information_view.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/cover_generator/cover_generator_view.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/image/image_card.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QAction>
#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class ProjectInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    StackWidget* content = nullptr;

    QScrollArea* parametersPage = nullptr;
    //
    Card* projectInfo = nullptr;
    QGridLayout* projectInfoLayout = nullptr;
    TextField* projectName = nullptr;
    TextField* projectLogline = nullptr;
    //
    CoverImageCard* projectCover = nullptr;

    CoverGeneratorView* generatorPage = nullptr;
};

ProjectInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new StackWidget(_parent))
    , parametersPage(new QScrollArea(_parent))
    , projectInfo(new Card(_parent))
    , projectInfoLayout(new QGridLayout)
    , projectName(new TextField(projectInfo))
    , projectLogline(new TextField(projectInfo))
    , projectCover(new CoverImageCard(projectInfo))
    , generatorPage(new CoverGeneratorView(_parent))
{
    content->setCurrentWidget(parametersPage);
    content->addWidget(generatorPage);

    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    parametersPage->setPalette(palette);
    parametersPage->setFrameShape(QFrame::NoFrame);
    parametersPage->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    parametersPage->setVerticalScrollBar(new ScrollBar);
    projectCover->setDecorationIcon(u8"\U000F02E9");

    projectInfoLayout->setContentsMargins({});
    projectInfoLayout->setSpacing(0);
    int row = 0;
    projectInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку над названием
    projectInfoLayout->addWidget(projectName, row++, 0);
    projectInfoLayout->addWidget(projectLogline, row++, 0);
    projectInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку под логлайном
    projectInfo->setContentLayout(projectInfoLayout);

    projectName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    projectLogline->setEnterMakesNewLine(true);
    UiHelper::initSpellingFor(projectLogline);

    QWidget* contentWidget = new QWidget;
    parametersPage->setWidget(contentWidget);
    parametersPage->setWidgetResizable(true);
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
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->projectName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->projectName->text()); });
    connect(d->projectLogline, &TextField::textChanged, this,
            [this] { emit loglineChanged(d->projectLogline->text()); });
    connect(d->projectCover, &CoverImageCard::generateCoverPressed, this,
            [this] { d->content->setCurrentWidget(d->generatorPage); });
    connect(d->projectCover, &CoverImageCard::imageChanged, this,
            &ProjectInformationView::coverChanged);
    connect(d->generatorPage, &CoverGeneratorView::savePressed, this, [this] {
        d->projectCover->setImage(d->generatorPage->coverImage());
        d->content->setCurrentWidget(d->parametersPage);
    });
    connect(d->generatorPage, &CoverGeneratorView::discardPressed, this,
            [this] { d->content->setCurrentWidget(d->parametersPage); });
}

ProjectInformationView::~ProjectInformationView() = default;

QWidget* ProjectInformationView::asQWidget()
{
    return this;
}

void ProjectInformationView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->projectName->setReadOnly(readOnly);
    d->projectLogline->setReadOnly(readOnly);
    d->projectCover->setReadOnly(readOnly);
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
    d->content->setBackgroundColor(Ui::DesignSystem::color().surface());

    d->parametersPage->widget()->layout()->setContentsMargins(
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


// ****


class CoverImageCard::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QAction* createCoverAction = nullptr;
};

CoverImageCard::Implementation::Implementation(QWidget* _parent)
    : createCoverAction(new QAction(_parent))
{
    createCoverAction->setIconText(u8"\U000F1353");
}


// **


CoverImageCard::CoverImageCard(QWidget* _parent)
    : ImageCard(_parent)
    , d(new Implementation(this))
{
    connect(d->createCoverAction, &QAction::triggered, this, &CoverImageCard::generateCoverPressed);
}

CoverImageCard::~CoverImageCard() = default;

QVector<QAction*> CoverImageCard::contextMenuActions() const
{
    auto actions = ImageCard::contextMenuActions();
    actions.prepend(d->createCoverAction);
    return actions;
}

void CoverImageCard::processReadOnlyChange()
{
    d->createCoverAction->setEnabled(!isReadOnly());
}

void CoverImageCard::updateTranslations()
{
    ImageCard::updateTranslations();

    d->createCoverAction->setText(tr("Create poster"));
}

} // namespace Ui
