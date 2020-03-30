#include "project_tree_delegate.h"

#include <business_layer/model/structure/structure_model.h>

#include <ui/design_system/design_system.h>

#include <QPainter>


namespace Ui {

ProjectTreeDelegate::ProjectTreeDelegate(QObject* _parent)
    : TreeDelegate(_parent)
{
}

void ProjectTreeDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    TreeDelegate::paint(_painter, _option, _index);

    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем иконку перехода в навигатор
    //
    if (opt.state.testFlag(QStyle::State_Selected)
        && _index.data(static_cast<int>(BusinessLayer::StructureModelDataRole::IsNavigatorAvailable)).toBool()) {
        const auto textColor = opt.palette.color(QPalette::HighlightedText);
        _painter->setPen(textColor);
        const QRectF backgroundRect = opt.rect;
        auto iconRect = QRectF(QPointF(backgroundRect.right()
                                       - Ui::DesignSystem::treeOneLineItem().iconSize().width()
                                       - Ui::DesignSystem::treeOneLineItem().margins().right(),
                                  backgroundRect.top()),
                          QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                 backgroundRect.height()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignCenter, u8"\uf142");
    }
}

} // namespace Ui
