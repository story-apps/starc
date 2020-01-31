#include "tree_delegate.h"

#include <ui/design_system/design_system.h>

#include <QPainter>


TreeDelegate::TreeDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

void TreeDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->setRenderHint(QPainter::Antialiasing, true);

    auto backgroundColor = opt.palette.color(QPalette::Base);
    auto textColor = opt.palette.color(QPalette::Text);

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = opt.rect;
    if (opt.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = opt.palette.color(QPalette::Highlight);
        textColor = opt.palette.color(QPalette::HighlightedText);
    } else if (opt.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = opt.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        iconRect = QRectF(QPointF(std::max(backgroundRect.left(),
                                           Ui::DesignSystem::treeOneLineItem().margins().left()),
                                  backgroundRect.top()),
                          QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                 backgroundRect.height()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignCenter, _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... текст
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    const qreal textLeft = iconRect.isValid()
                           ? iconRect.right() + Ui::DesignSystem::treeOneLineItem().spacing()
                           : backgroundRect.left() + Ui::DesignSystem::treeOneLineItem().margins().left();
    const QRectF textRect(QPointF(textLeft,
                                  backgroundRect.top()),
                          QSizeF(backgroundRect.right() - textLeft - Ui::DesignSystem::treeOneLineItem().margins().right(),
                                 backgroundRect.height()));
    const auto text = _painter->fontMetrics().elidedText(_index.data().toString(), Qt::ElideRight, static_cast<int>(textRect.width()));
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

QSize TreeDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_option)
    Q_UNUSED(_index)

    return {50, static_cast<int>(Ui::DesignSystem::treeOneLineItem().height())};
}
