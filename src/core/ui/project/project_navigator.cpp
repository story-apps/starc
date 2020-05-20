#include "project_navigator.h"

#include "project_tree_delegate.h"

#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>

#include <domain/document_object.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QUuid>
#include <QVBoxLayout>


namespace Ui
{

class ProjectNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Widget* navigatorPage = nullptr;
    Tree* tree = nullptr;
    ProjectTreeDelegate* treeDelegate = nullptr;
    ContextMenu* contextMenu = nullptr;
    TextField* filter = nullptr;
};

ProjectNavigator::Implementation::Implementation(QWidget* _parent)
    : navigatorPage(new Widget(_parent)),
      tree(new Tree(_parent)),
      treeDelegate(new ProjectTreeDelegate(tree)),
      contextMenu(new ContextMenu(tree)),
      filter(new TextField(_parent))
{
    tree->setDragDropEnabled(true);
    tree->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    filter->hide();
}


// ****


ProjectNavigator::ProjectNavigator(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setAnimationType(AnimationType::Slide);

    QHBoxLayout* filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(0);
    filterLayout->setContentsMargins({});

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);
    layout->addWidget(d->filter);
    d->navigatorPage->setLayout(layout);
    showProjectNavigator();

    connect(d->tree, &Tree::currentIndexChanged, this, &ProjectNavigator::itemSelected);
    connect(d->tree, &Tree::doubleClicked, this, &ProjectNavigator::itemDoubleClicked);
    connect(d->tree, &Tree::customContextMenuRequested, this, [this] (const QPoint& _pos) {
        //
        // Уведомляем менеджер, что необходимо обновить модель контекстного меню
        //
        emit contextMenuUpdateRequested(d->tree->indexAt(_pos));

        //
        // Отображаем контекстное меню с обновлённой моделью
        //
        d->contextMenu->showContextMenu(d->tree->mapToGlobal(_pos));
    });

    connect(d->contextMenu, &ContextMenu::clicked, d->contextMenu, &ContextMenu::hide);
    connect(d->contextMenu, &ContextMenu::clicked, this, [this] (const QModelIndex& _contextMenuIndex) {
        emit contextMenuItemClicked(_contextMenuIndex);
    });
}

void ProjectNavigator::setModel(QAbstractItemModel* _model)
{
    d->tree->setModel(_model);
}

void ProjectNavigator::setContextMenuModel(QAbstractItemModel* _model)
{
    d->contextMenu->setModel(_model);
}

QVariant ProjectNavigator::saveState() const
{
    return d->tree->saveState();
}

void ProjectNavigator::restoreState(const QVariant& _state)
{
    d->tree->restoreState(_state);
}

QModelIndex ProjectNavigator::currentIndex() const
{
    return d->tree->currentIndex();
}

void ProjectNavigator::showProjectNavigator()
{
    setCurrentWidget(d->navigatorPage);
}

bool ProjectNavigator::isProjectNavigatorShown() const
{
    return currentWidget() == d->navigatorPage;
}

void ProjectNavigator::updateTranslations()
{
    d->filter->setLabel(tr("Filter"));
}

ProjectNavigator::~ProjectNavigator() = default;

void ProjectNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    d->navigatorPage->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());
    d->tree->setItemDelegate(d->treeDelegate);
    d->contextMenu->setBackgroundColor(DesignSystem::color().background());
    d->contextMenu->setTextColor(DesignSystem::color().onBackground());
    d->filter->setBackgroundColor(DesignSystem::color().primary());
    d->filter->setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui
