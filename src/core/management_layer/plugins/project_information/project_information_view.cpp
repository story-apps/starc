#include "project_information_view.h"

#include "cover.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui
{

class ProjectInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* projectInfo = nullptr;
    QGridLayout* projectInfoLayout = nullptr;
    TextField* projectName = nullptr;
    TextField* projectLogline = nullptr;
    Cover* projectCover = nullptr;
};

ProjectInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      projectInfo(new Card(_parent)),
      projectInfoLayout(new QGridLayout),
      projectName(new TextField(projectInfo)),
      projectLogline(new TextField(projectInfo)),
      projectCover(new Cover(projectInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setVerticalScrollBar(new ScrollBar);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    projectInfoLayout->setContentsMargins({});
    projectInfoLayout->setSpacing(0);
    projectInfoLayout->setRowMinimumHeight(0, 1); // добавляем пустую строку над названием
    projectInfoLayout->addWidget(projectName, 1, 0);
    projectInfoLayout->addWidget(projectLogline, 2, 0);
    projectInfoLayout->setRowMinimumHeight(3, 1); // добавляем пустую строку под логлайном
    projectInfoLayout->setRowStretch(3, 1);
    projectInfoLayout->addWidget(projectCover, 0, 1, 4, 1, Qt::AlignTop);
    projectInfoLayout->setColumnStretch(0, 1);
    projectInfo->setLayoutReimpl(projectInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(projectInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ProjectInformationView::ProjectInformationView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->projectName, &TextField::textChanged, this, [this] {
        emit nameChanged(d->projectName->text());
    });
    connect(d->projectLogline, &TextField::textChanged, this, [this] {
        emit loglineChanged(d->projectLogline->text());
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ProjectInformationView::~ProjectInformationView() = default;

void ProjectInformationView::setName(const QString& _name)
{
    d->projectName->setText(_name);
}

void ProjectInformationView::setLogline(const QString& _logline)
{
    d->projectLogline->setText(_logline);
}

void ProjectInformationView::setCover(const QPixmap& _cover)
{
    d->projectCover->setCover(_cover);
}

void ProjectInformationView::updateTranslations()
{
    d->projectName->setLabel(tr("Project name"));
    d->projectLogline->setLabel(tr("Short description"));
    d->projectLogline->setHelper(tr("Tagline, logline or something similar"));
}

void ProjectInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().topContentMargin(),
                          Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24())
                .toMargins());

    d->projectInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->projectName, d->projectLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().background());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->projectInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->projectInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->projectInfoLayout->setRowMinimumHeight(3, static_cast<int>(Ui::DesignSystem::layout().px8()));
}

} // namespace Ui
