#include "bookmark_delegate.h"

#include "bookmarks_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemView>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>

using BusinessLayer::BookmarksModel;


namespace Ui {

BookmarkDelegate::BookmarkDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

void BookmarkDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
                             const QModelIndex& _index) const
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
    _painter->setPen(_index.data(BookmarksModel::BookmarkColorRole).value<QColor>());
    _painter->setFont(Ui::DesignSystem::font().iconsMid());
    const QRectF iconRect(QPointF(Ui::DesignSystem::layout().px16(),
                                  backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                          Ui::DesignSystem::treeOneLineItem().iconSize());
    _painter->drawText(iconRect, Qt::AlignCenter, u8"\U000F00C0");

    //
    // ... заголовок
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal textLeft = iconRect.right() + Ui::DesignSystem::layout().px12();
    const qreal textWidth = backgroundRect.right() - textLeft - Ui::DesignSystem::layout().px12();
    const QRectF textRect(QPointF(textLeft, iconRect.top()), QSizeF(textWidth, iconRect.height()));
    const auto bookmarkName = _index.data(BookmarksModel::BookmarkNameRole).toString();
    const auto text
        = _painter->fontMetrics().elidedText(bookmarkName.isEmpty() ? tr("") : bookmarkName,
                                             Qt::ElideRight, static_cast<int>(textRect.width()));
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

QSize BookmarkDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                 const QModelIndex& _index) const
{
    Q_UNUSED(_index)

    //
    // Ширина
    //
    int width = _option.rect.width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= Ui::DesignSystem::layout().px8() + Ui::DesignSystem::layout().px16()
        + Ui::DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    // ... высота заголовка: отступ сверху + высота иконки + отступ снизу
    //
    const int height = Ui::DesignSystem::layout().px16()
        + Ui::DesignSystem::treeOneLineItem().iconSize().height()
        + Ui::DesignSystem::layout().px16();

    return { width, height };
}

} // namespace Ui
