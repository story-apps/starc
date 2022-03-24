#include "tree.h"

#include "tree_delegate.h"
#include "tree_header_view.h"
#include "tree_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>

#include <QVBoxLayout>


class Tree::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    TreeView* tree = nullptr;
    QHeaderView* header = nullptr;
    TreeDelegate* delegate = nullptr;
    ScrollBar* treeScrollBar = nullptr;
};

Tree::Implementation::Implementation(QWidget* _parent)
    : tree(new TreeView(_parent))
    , header(new TreeHeaderView(_parent))
    , delegate(new TreeDelegate(_parent))
    , treeScrollBar(new ScrollBar(tree))
{
    tree->setHeader(header);
    tree->setHeaderHidden(true);
    tree->setAnimated(true);
    tree->setMouseTracking(true);
    tree->setFrameShape(QFrame::NoFrame);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setVerticalScrollBar(treeScrollBar);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tree->setItemDelegate(delegate);

    header->setSectionResizeMode(QHeaderView::Stretch);
}


// ****


Tree::Tree(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);

    connect(d->tree, &QTreeView::clicked, this, &Tree::clicked);
    connect(d->tree, &QTreeView::doubleClicked, this, &Tree::doubleClicked);

    designSystemChangeEvent(nullptr);
}

Tree::~Tree() = default;

void Tree::setModel(QAbstractItemModel* _model)
{
    if (model() != nullptr) {
        disconnect(d->tree->selectionModel(), &QItemSelectionModel::currentChanged, this,
                   &Tree::currentIndexChanged);
    }

    d->tree->setModel(_model);

    if (model() != nullptr) {
        connect(d->tree->selectionModel(), &QItemSelectionModel::currentChanged, this,
                &Tree::currentIndexChanged);
    }
}

QAbstractItemModel* Tree::model() const
{
    return d->tree->model();
}

void Tree::setRootIsDecorated(bool _decorated)
{
    d->tree->setRootIsDecorated(_decorated);
}

void Tree::setScrollBarVisible(bool _visible)
{
    d->tree->setVerticalScrollBarPolicy(_visible ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}

QScrollBar* Tree::verticalScrollBar() const
{
    return d->tree->verticalScrollBar();
}

void Tree::setHeaderVisible(bool _visible)
{
    d->tree->setHeaderHidden(!_visible);
}

void Tree::setColumnWidth(int _column, int _width)
{
    d->header->setSectionResizeMode(_column, QHeaderView::Fixed);
    d->header->resizeSection(_column, _width);
}

void Tree::setDragDropEnabled(bool _enabled)
{
    d->tree->setAcceptDrops(_enabled);
    d->tree->setDragEnabled(_enabled);
    d->tree->setDragDropMode(_enabled ? QAbstractItemView::DragDrop
                                      : QAbstractItemView::NoDragDrop);
    d->tree->setDropIndicatorShown(_enabled);
}

void Tree::setSelectionMode(QAbstractItemView::SelectionMode _mode)
{
    d->tree->setSelectionMode(_mode);
}

int Tree::sizeHintForColumn(int _column) const
{
    return d->tree->sizeHintForColumn(_column);
}

void Tree::setItemDelegate(QAbstractItemDelegate* _delegate)
{
    d->tree->setItemDelegate(_delegate);
}

void Tree::setItemDelegateForColumn(int _column, QAbstractItemDelegate* _delegate)
{
    d->tree->setItemDelegateForColumn(_column, _delegate);
}

void Tree::setCurrentIndex(const QModelIndex& _index)
{
    if (d->tree->selectionModel() != nullptr
        && d->tree->selectionModel()->selectedIndexes().contains(_index)) {
        return;
    }

    d->tree->clearSelection();
    d->tree->setCurrentIndex(_index);
}

QModelIndex Tree::currentIndex() const
{
    return d->tree->currentIndex();
}

QModelIndex Tree::indexAt(const QPoint& _pos) const
{
    return d->tree->indexAt(_pos);
}

QModelIndexList Tree::selectedIndexes() const
{
    return d->tree->selectionModel()->selectedIndexes();
}

void Tree::setExpandsOnDoubleClick(bool _expand)
{
    d->tree->setExpandsOnDoubleClick(_expand);
}

bool Tree::isExpanded(const QModelIndex& _index) const
{
    return d->tree->isExpanded(_index);
}

void Tree::expand(const QModelIndex& _index)
{
    d->tree->expand(_index);
}

void Tree::expandAll()
{
    d->tree->expandAll();
}

void Tree::setAutoAdjustSize(bool _auto)
{
    d->tree->setAutoAdjustSize(_auto);
}

QRect Tree::visualRect(const QModelIndex& _index) const
{
    return d->tree->visualRect(_index);
}

void Tree::setHeader(QHeaderView* _headerView)
{
    if (d->header) {
        d->header->deleteLater();
    }

    d->header = _headerView;
    d->header->setSectionResizeMode(QHeaderView::Stretch);
    d->tree->setHeader(_headerView);
}

QHeaderView* Tree::headerView() const
{
    return d->tree->header();
}

void Tree::restoreState(const QVariant& _state)
{
    d->tree->restoreState(_state);
}

QVariant Tree::saveState() const
{
    return d->tree->saveState();
}

void Tree::processBackgroundColorChange()
{
    designSystemChangeEvent(nullptr);
}

void Tree::processTextColorChange()
{
    designSystemChangeEvent(nullptr);
}

void Tree::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    QPalette palette = d->tree->palette();
    palette.setColor(QPalette::Base, backgroundColor());
    auto alternateBaseColor = textColor();
    alternateBaseColor.setAlphaF(Ui::DesignSystem::hoverBackgroundOpacity());
    palette.setColor(QPalette::AlternateBase, alternateBaseColor);
    palette.setColor(QPalette::Text, textColor());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::tree().selectionColor());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().secondary());
    d->tree->setPalette(palette);
    d->tree->setIndentation(static_cast<int>(Ui::DesignSystem::tree().indicatorWidth()));
    auto lastDelegate = d->tree->itemDelegate();
    d->tree->setItemDelegate(nullptr);
    d->tree->setItemDelegate(lastDelegate);

    d->header->setPalette(palette);

    d->treeScrollBar->setBackgroundColor(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::elevationEndOpacity()));
    d->treeScrollBar->setHandleColor(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::focusBackgroundOpacity()));
}
