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

    Card* userInfo = nullptr;
    QGridLayout* userInfoLayout = nullptr;
    TextField* userName = nullptr;
    TextField* userLogline = nullptr;
    Cover* avatar = nullptr;
};

ProjectInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      userInfo(new Card(_parent)),
      userInfoLayout(new QGridLayout),
      userName(new TextField(userInfo)),
      userLogline(new TextField(userInfo)),
      avatar(new Cover(userInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setVerticalScrollBar(new ScrollBar);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    userInfoLayout->setContentsMargins({});
    userInfoLayout->setSpacing(0);
    userInfoLayout->setRowMinimumHeight(0, 1); // добавляем пустую строку над названием
    userInfoLayout->addWidget(userName, 1, 0);
    userInfoLayout->addWidget(userLogline, 2, 0);
    userInfoLayout->setRowMinimumHeight(3, 1); // добавляем пустую строку под логлайном
    userInfoLayout->setRowStretch(3, 1);
    userInfoLayout->addWidget(avatar, 0, 1, 4, 1, Qt::AlignTop);
    userInfoLayout->setColumnStretch(0, 1);
    userInfo->setLayoutReimpl(userInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(userInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ProjectInformationView::ProjectInformationView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{d->avatar->setCover(QPixmap(":/images/movie-poster"));
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ProjectInformationView::~ProjectInformationView() = default;

void ProjectInformationView::updateTranslations()
{
    d->userName->setLabel(tr("Project name"));
    d->userLogline->setLabel(tr("Short description"));
    d->userLogline->setHelper(tr("Tagline, logline or something similar"));
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

    d->userInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->userName, d->userLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().background());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->userInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->userInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->userInfoLayout->setRowMinimumHeight(3, static_cast<int>(Ui::DesignSystem::layout().px8()));
}

} // namespace Ui
