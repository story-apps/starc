#include "project_navigator.h"

#include "project_tree_delegate.h"

#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QUuid>
#include <QVBoxLayout>


namespace Ui {

class ProjectNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Widget* navigatorPage = nullptr;
    Tree* tree = nullptr;
    ProjectTreeDelegate* treeDelegate = nullptr;
    ContextMenu* contextMenu = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* addDocumentButton = nullptr;
};

ProjectNavigator::Implementation::Implementation(QWidget* _parent)
    : navigatorPage(new Widget(_parent))
    , tree(new Tree(_parent))
    , treeDelegate(new ProjectTreeDelegate(tree))
    , contextMenu(new ContextMenu(tree))
    , buttonsLayout(new QHBoxLayout)
    , addDocumentButton(new Button(_parent))
{
    tree->setDragDropEnabled(true);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    addDocumentButton->setFocusPolicy(Qt::NoFocus);
    addDocumentButton->setIcon(u8"\U000f0415");

    new Shadow(Qt::TopEdge, tree);
    new Shadow(Qt::BottomEdge, tree);
}


// ****


ProjectNavigator::ProjectNavigator(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(AnimationType::Slide);

    d->buttonsLayout->setContentsMargins({});
    d->buttonsLayout->setSpacing(0);
    d->buttonsLayout->addWidget(d->addDocumentButton);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);
    layout->addLayout(d->buttonsLayout);
    d->navigatorPage->setLayout(layout);
    showProjectNavigator();

    connect(d->tree, &Tree::currentIndexChanged, this, &ProjectNavigator::itemSelected);
    connect(d->tree, &Tree::doubleClicked, this, &ProjectNavigator::itemDoubleClicked);
    connect(d->tree, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        //
        // Уведомляем менеджер, что необходимо обновить модель контекстного меню
        //
        emit contextMenuUpdateRequested(d->tree->indexAt(_pos));

        //
        // Отображаем контекстное меню с обновлённой моделью
        //
        d->contextMenu->showContextMenu(d->tree->mapToGlobal(_pos));
    });

    connect(d->addDocumentButton, &Button::clicked, this, &ProjectNavigator::addDocumentClicked);
}

void ProjectNavigator::setModel(QAbstractItemModel* _model)
{
    d->tree->setModel(_model);
}

void ProjectNavigator::setContextMenuActions(const QVector<QAction*>& _actions)
{
    d->contextMenu->setActions(_actions);
}

QVariant ProjectNavigator::saveState() const
{
    return d->tree->saveState();
}

void ProjectNavigator::restoreState(const QVariant& _state)
{
    if (!_state.isValid()) {
        d->tree->setCurrentIndex(d->tree->model()->index(0, 0));
        return;
    }

    d->tree->restoreState(_state);
}

void ProjectNavigator::setCurrentIndex(const QModelIndex& _index)
{
    d->tree->setCurrentIndex(_index);
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
    d->addDocumentButton->setText(tr("Add document"));
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


    d->buttonsLayout->setContentsMargins(
        DesignSystem::layout().px12(), DesignSystem::layout().px12(), DesignSystem::layout().px12(),
        DesignSystem::layout().px12());
    d->addDocumentButton->setBackgroundColor(DesignSystem::color().secondary());
    d->addDocumentButton->setTextColor(DesignSystem::color().secondary());
}

} // namespace Ui
