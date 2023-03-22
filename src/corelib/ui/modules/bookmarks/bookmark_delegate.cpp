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
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

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
        textColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... цвет закладки
    //
    const auto itemColor = _index.data(BookmarksModel::BookmarkColorRole);
    QRectF itemColorRect;
    if (!itemColor.isNull() && itemColor.canConvert<QColor>()) {
        const QColor color = itemColor.value<QColor>();
        if (color.isValid()) {
            auto fullIndicatorWidth = [_index] {
                int level = 0;
                auto index = _index;
                while (index.isValid()) {
                    ++level;
                    index = index.parent();
                }
                return level * DesignSystem::tree().indicatorWidth();
            };
            const auto backgroundRect = _option.rect;
            itemColorRect = QRectF(isLeftToRight ? 0.0
                                                 : (backgroundRect.right() + fullIndicatorWidth()
                                                    - DesignSystem::layout().px4()),
                                   backgroundRect.top(), DesignSystem::layout().px4(),
                                   backgroundRect.height());
            _painter->fillRect(itemColorRect, color);
        }
    }

    //
    // ... заголовок
    //
    _painter->setFont(DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal textLeft = isLeftToRight ? (itemColorRect.right() + DesignSystem::layout().px16())
                                         : DesignSystem::treeOneLineItem().margins().left();
    const qreal textWidth = isLeftToRight
        ? (backgroundRect.right() - textLeft - DesignSystem::treeOneLineItem().margins().right())
        : (backgroundRect.right() + DesignSystem::tree().indicatorWidth() - textLeft
           - itemColorRect.width() - DesignSystem::treeOneLineItem().margins().right());
    const QRectF headingRect(
        QPointF(textLeft, backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        QSizeF(textWidth, DesignSystem::treeOneLineItem().contentHeight()));
    auto header
        = TextHelper::smartToUpper(_index.data(BookmarksModel::BookmarkItemTextRole).toString());
    header = _painter->fontMetrics().elidedText(header, Qt::ElideRight,
                                                static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, header);

    //
    // ... текст закладки
    //
    const auto bookmarkName = _index.data(BookmarksModel::BookmarkNameRole).toString();
    if (!bookmarkName.isEmpty()) {
        const QRectF bookamrkNameRect(
            QPointF(headingRect.left(), headingRect.bottom() + DesignSystem::compactLayout().px8()),
            QSizeF(headingRect.width(),
                   TextHelper::heightForWidth(bookmarkName, DesignSystem::font().body2(),
                                              headingRect.width())));
        _painter->setFont(DesignSystem::font().body2());
        _painter->setPen(textColor);
        QTextOption commentTextOption;
        commentTextOption.setAlignment(isLeftToRight ? Qt::AlignLeft : Qt::AlignRight);
        commentTextOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        _painter->drawText(bookamrkNameRect, bookmarkName, commentTextOption);
    }
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
    width -= DesignSystem::layout().px8() + DesignSystem::layout().px16()
        + DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const auto bookmarkName = _index.data(BookmarksModel::BookmarkNameRole).toString();
    int height = DesignSystem::treeOneLineItem().height();
    if (!bookmarkName.isEmpty()) {
        height += DesignSystem::compactLayout().px8()
            + TextHelper::heightForWidth(bookmarkName, DesignSystem::font().body2(), width);
    }
    return { width, height };
}

} // namespace Ui
