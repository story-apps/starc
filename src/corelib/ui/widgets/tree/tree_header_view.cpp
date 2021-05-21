#include "tree_header_view.h"

#include <ui/design_system/design_system.h>

#include <QPainter>


TreeHeaderView::TreeHeaderView(QWidget* _parent)
    : QHeaderView(Qt::Horizontal, _parent)
{
}

QSize TreeHeaderView::sizeHint() const
{
    return QSize(QHeaderView::sizeHint().width(), Ui::DesignSystem::treeOneLineItem().height());
}

void TreeHeaderView::paintSection(QPainter* _painter, const QRect& _rect, int _section) const
{
    const auto backgroundColor = palette().color(QPalette::Base);
    _painter->fillRect(_rect, backgroundColor);

    auto textColor = palette().color(QPalette::Text);
    textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    auto textRect = _rect.adjusted(_section == 0
                                   ? Ui::DesignSystem::tree().indicatorWidth()
                                   : Ui::DesignSystem::treeOneLineItem().margins().left()
                                   ,
                                   0, 0, 0);
    if (model() && model()->index(0, _section).data(Qt::DecorationRole).isValid()) {
        textRect.adjust(Ui::DesignSystem::treeOneLineItem().iconSize().width()
                        + Ui::DesignSystem::treeOneLineItem().spacing(),
                        0, 0, 0);
    }
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, model()->headerData(_section, orientation()).toString());
}
