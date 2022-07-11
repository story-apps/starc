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
#include <utils/shugar.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QScrollBar>
#include <QShortcut>
#include <QToolTip>
#include <QUuid>
#include <QVBoxLayout>

namespace Ui {

class ProjectNavigator::Implementation
{
public:
    explicit Implementation(ProjectNavigator* _parent);

    /**
     * @brief Находится ли заданная позиция над иконкой отображения навигатора по документу
     */
    bool isOnDocumentNavigatorButton(const QPoint& _position) const;


    ProjectNavigator* q = nullptr;

    bool isReadOnly = false;

    Widget* navigatorPage = nullptr;
    Tree* tree = nullptr;
    ProjectTreeDelegate* treeDelegate = nullptr;
    ContextMenu* contextMenu = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* addDocumentButton = nullptr;
    QShortcut* addDocumentShortcut = nullptr;
    Button* emptyRecycleBinButton = nullptr;
};

ProjectNavigator::Implementation::Implementation(ProjectNavigator* _parent)
    : q(_parent)
    , navigatorPage(new Widget(_parent))
    , tree(new Tree(_parent))
    , treeDelegate(new ProjectTreeDelegate(tree))
    , contextMenu(new ContextMenu(tree))
    , buttonsLayout(new QHBoxLayout)
    , addDocumentButton(new Button(_parent))
    , addDocumentShortcut(new QShortcut(_parent))
    , emptyRecycleBinButton(new Button(_parent))
{
    tree->setDragDropEnabled(true);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    tree->setExpandsOnDoubleClick(false);
    addDocumentButton->setFocusPolicy(Qt::NoFocus);
    addDocumentButton->setIcon(u8"\U000f0415");
    addDocumentShortcut->setKey(QKeySequence::New);
    emptyRecycleBinButton->setFocusPolicy(Qt::NoFocus);
    emptyRecycleBinButton->setIcon(u8"\U000F05E8");
    emptyRecycleBinButton->hide();

    new Shadow(Qt::TopEdge, tree);
    new Shadow(Qt::BottomEdge, tree);
}

bool ProjectNavigator::Implementation::isOnDocumentNavigatorButton(const QPoint& _position) const
{
    const auto isNavigatorAvailable
        = q->currentIndex()
              .data(static_cast<int>(BusinessLayer::StructureModelDataRole::IsNavigatorAvailable))
              .toBool();
    if (!isNavigatorAvailable) {
        return false;
    }

    return tree->isOnItemTrilingIcon(_position);
}


// ****


ProjectNavigator::ProjectNavigator(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(AnimationType::Slide);

    d->tree->installEventFilter(this);
    d->buttonsLayout->setContentsMargins({});
    d->buttonsLayout->setSpacing(0);
    d->buttonsLayout->addWidget(d->addDocumentButton);
    d->buttonsLayout->addWidget(d->emptyRecycleBinButton);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);
    layout->addLayout(d->buttonsLayout);
    d->navigatorPage->setLayout(layout);
    showProjectNavigator();

    connect(d->tree, &Tree::currentIndexChanged, this, &ProjectNavigator::itemSelected);
    connect(d->tree, &Tree::clicked, this, [this](const QModelIndex& _index, bool _firstClick) {
        const auto clickPosition = d->tree->mapFromGlobal(QCursor::pos());
        if (!_firstClick && d->isOnDocumentNavigatorButton(clickPosition)) {
            emit itemNavigationRequested(_index);
        }
    });
    connect(d->tree, &Tree::doubleClicked, this, [this](const QModelIndex& _index) {
        if (d->tree->model()->rowCount(_index) > 0 && !d->tree->isExpanded(_index)) {
            d->tree->expand(_index);
        } else {
            emit itemDoubleClicked(_index);
        }
    });
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
    connect(d->addDocumentShortcut, &QShortcut::activated, this, [this] {
        if (d->addDocumentButton->isVisible()) {
            emit addDocumentClicked();
        }
    });
    connect(d->emptyRecycleBinButton, &Button::clicked, this,
            &ProjectNavigator::emptyRecycleBinClicked);
}

ProjectNavigator::~ProjectNavigator() = default;

void ProjectNavigator::setReadOnly(bool _readOnly)
{
    if (d->isReadOnly == _readOnly) {
        return;
    }

    d->isReadOnly = _readOnly;
    const auto enabled = !d->isReadOnly;
    d->tree->setDragDropEnabled(enabled);
    d->addDocumentButton->setEnabled(enabled);
    d->addDocumentShortcut->setEnabled(enabled);
    d->emptyRecycleBinButton->setEnabled(enabled);
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

void ProjectNavigator::restoreState(bool _isNewProject, const QVariant& _state)
{
    //
    // Если проект новый, то раскрываем только последний документ
    //
    if (_isNewProject) {
        auto model = d->tree->model();
        for (int row = model->rowCount() - 1; row >= 0; --row) {
            const auto index = model->index(row, 0);
            if (model->rowCount(index) > 0) {
                d->tree->expand(index);
                break;
            }
        }
    }

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

void ProjectNavigator::showButton(ActionButton _type)
{
    switch (_type) {
    case ActionButton::AddDocument: {
        d->emptyRecycleBinButton->hide();
        d->addDocumentButton->show();
        break;
    }

    case ActionButton::EmptyRecycleBin: {
        d->addDocumentButton->hide();
        d->emptyRecycleBinButton->show();
        break;
    }
    }
}

void ProjectNavigator::setButtonEnabled(bool _enabled)
{
    d->addDocumentButton->setEnabled(!d->isReadOnly && _enabled);
    d->emptyRecycleBinButton->setEnabled(!d->isReadOnly && _enabled);
}

bool ProjectNavigator::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->tree && _event->type() == QEvent::ToolTip) {
        QHelpEvent* event = static_cast<QHelpEvent*>(_event);
        if (d->isOnDocumentNavigatorButton(event->pos())) {
            QToolTip::showText(event->globalPos(), tr("Show document navigator"));
        }
    }
    return StackWidget::eventFilter(_watched, _event);
}

void ProjectNavigator::updateTranslations()
{
    d->addDocumentButton->setText(tr("Add document"));
    d->emptyRecycleBinButton->setText(tr("Empty recycle bin"));
}

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
    d->emptyRecycleBinButton->setBackgroundColor(DesignSystem::color().secondary());
    d->emptyRecycleBinButton->setTextColor(DesignSystem::color().secondary());
}

} // namespace Ui
