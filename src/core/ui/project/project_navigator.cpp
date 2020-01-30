#include "project_navigator.h"

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

    Tree* tree = nullptr;
    ContextMenu* contexMenu = nullptr;
    TextField* filterText = nullptr;
};

ProjectNavigator::Implementation::Implementation(QWidget* _parent)
    : tree(new Tree(_parent)),
      contexMenu(new ContextMenu(tree)),
      filterText(new TextField(_parent))
{
    tree->setDragDropEnabled(true);
    tree->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}


// ****


ProjectNavigator::ProjectNavigator(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QHBoxLayout* filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(0);
    filterLayout->setContentsMargins({});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);
    layout->addWidget(d->filterText);

    connect(d->tree, &Tree::currentIndexChanged, this, &ProjectNavigator::itemSelected);
    connect(d->tree, &Tree::customContextMenuRequested, this, [this] (const QPoint& _pos) {
        //
        // Уведомляем менеджер, что необходимо обновить модель контекстного меню
        //
        emit contextMenuUpdateRequested(d->tree->indexAt(_pos));

        //
        // Отображаем контекстное меню с обновлённой моделью
        //
        d->contexMenu->showContextMenu(d->tree->mapToGlobal(_pos));
    });
}

void ProjectNavigator::setModel(QAbstractItemModel* _model)
{
    d->tree->setModel(_model);
}

void ProjectNavigator::setContextMenuModel(QAbstractItemModel* _model)
{
    d->contexMenu->setModel(_model);
}

QVariant ProjectNavigator::saveState() const
{
    return d->tree->saveState();
}

void ProjectNavigator::restoreState(const QVariant& _state)
{
    d->tree->restoreState(_state);
}

void ProjectNavigator::updateTranslations()
{
    d->filterText->setLabel(tr("Filter"));
}

ProjectNavigator::~ProjectNavigator() = default;

void ProjectNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    auto layout = this->layout();
    layout->setContentsMargins(0, 0, 0, static_cast<int>(Ui::DesignSystem::layout().px24()));

    setBackgroundColor(DesignSystem::color().primary());
    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());
    d->contexMenu->setBackgroundColor(DesignSystem::color().background());
    d->contexMenu->setTextColor(DesignSystem::color().onBackground());
    d->filterText->setBackgroundColor(DesignSystem::color().primary());
    d->filterText->setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui
