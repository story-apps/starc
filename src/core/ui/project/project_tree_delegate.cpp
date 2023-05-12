#include "project_tree_delegate.h"

#include <business_layer/model/structure/structure_model.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/icon_helper.h>

#include <QPainter>


namespace Ui {

ProjectTreeDelegate::ProjectTreeDelegate(QObject* _parent)
    : TreeDelegate(_parent)
{
}

void ProjectTreeDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
                                const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Если есть навигатор, немного уменьшаем область для отрисовки текста
    //
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const auto hasDecoration = opt.state.testFlag(QStyle::State_Selected)
        && _index.data(BusinessLayer::StructureModelDataRole::IsNavigatorAvailable).toBool();
    const auto decorationWidth = Ui::DesignSystem::treeOneLineItem().iconSize().width();
    const auto canDrawDecoration = opt.rect.width() > decorationWidth * 2;
    if (hasDecoration && canDrawDecoration) {
        opt.rect = opt.rect.adjusted(isLeftToRight ? 0 : decorationWidth, 0,
                                     isLeftToRight ? -1 * decorationWidth : 0, 0);
    }

    //
    // Рисуем базовую информацию
    //
    TreeDelegate::paint(_painter, opt, _index);

    //
    // Рисуем иконку перехода в навигатор
    //
    if (hasDecoration && canDrawDecoration) {
        //
        // Заливаем область под иконкой самостоятельно
        //
        _painter->fillRect(QRect(isLeftToRight ? opt.rect.topRight()
                                         + QPoint(1, 0) // 1 пиксель, чтобы области не накладывались
                                               : opt.rect.topLeft() - QPoint(decorationWidth, 0),
                                 QSize(decorationWidth, opt.rect.height())),
                           opt.palette.color(QPalette::Highlight));

        //
        // Рисуем декорацию
        //
        const auto textColor = opt.palette.color(QPalette::HighlightedText);
        _painter->setPen(textColor);
        const QRectF backgroundRect = opt.rect.adjusted(isLeftToRight ? 0 : -1 * decorationWidth, 0,
                                                        isLeftToRight ? decorationWidth : 0, 0);
        auto iconRect = QRectF(
            QPointF(isLeftToRight ? (backgroundRect.right()
                                     - Ui::DesignSystem::treeOneLineItem().iconSize().width()
                                     - Ui::DesignSystem::treeOneLineItem().margins().right())
                                  : (backgroundRect.left()
                                     + Ui::DesignSystem::treeOneLineItem().margins().left()),
                    backgroundRect.top()),
            QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                   backgroundRect.height()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignCenter, IconHelper::chevronRight());
    }
}

} // namespace Ui
