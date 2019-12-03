#include "tree.h"
#include "tree_delegate.h"

#include <ui/design_system/design_system.h>

#include <QTreeView>
#include <QVBoxLayout>


class Tree::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QTreeView* tree = nullptr;
    TreeDelegate* delegate = nullptr;
};

Tree::Implementation::Implementation(QWidget* _parent)
    : tree(new QTreeView(_parent)),
      delegate(new TreeDelegate(_parent))
{
    tree->setHeaderHidden(true);
    tree->setFrameShape(QFrame::NoFrame);
    tree->setItemDelegate(delegate);
}


// ****


Tree::Tree(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);

    designSystemChangeEvent(nullptr);
}

void Tree::setModel(QAbstractItemModel* _model)
{
    d->tree->setModel(_model);
}

void Tree::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    QPalette palette = d->tree->palette();
    palette.setColor(QPalette::Base, backgroundColor());
    palette.setColor(QPalette::Text, textColor());
    palette.setColor(QPalette::Highlight, Qt::transparent);
    d->tree->setPalette(palette);
    d->tree->setIndentation(static_cast<int>(Ui::DesignSystem::tree().indicatorWidth()));
}

Tree::~Tree() = default;
