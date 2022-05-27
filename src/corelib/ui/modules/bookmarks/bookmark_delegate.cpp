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
        textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
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
                return level * Ui::DesignSystem::tree().indicatorWidth();
            };
            const auto backgroundRect = _option.rect;
            itemColorRect = QRectF(isLeftToRight ? 0.0
                                                 : (backgroundRect.right() + fullIndicatorWidth()
                                                    - Ui::DesignSystem::layout().px4()),
                                   backgroundRect.top(), Ui::DesignSystem::layout().px4(),
                                   backgroundRect.height());
            _painter->fillRect(itemColorRect, color);
        }
    }

    //
    // ... заголовок
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal textLeft = isLeftToRight
        ? (itemColorRect.right() + Ui::DesignSystem::layout().px16())
        : Ui::DesignSystem::layout().px12();
    const qreal textWidth = isLeftToRight
        ? (backgroundRect.right() - textLeft - Ui::DesignSystem::layout().px12())
        : (backgroundRect.right() + Ui::DesignSystem::tree().indicatorWidth() - textLeft
           - itemColorRect.width() - Ui::DesignSystem::layout().px16());
    const QRectF headingRect(
        QPointF(textLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
        QSizeF(textWidth, Ui::DesignSystem::treeOneLineItem().iconSize().height()));
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
            QPointF(headingRect.left(), headingRect.bottom() + Ui::DesignSystem::layout().px4()),
            QSizeF(headingRect.width(),
                   TextHelper::heightForWidth(bookmarkName, Ui::DesignSystem::font().body2(),
                                              headingRect.width())));
        _painter->setFont(Ui::DesignSystem::font().body2());
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
    width -= Ui::DesignSystem::layout().px8() + Ui::DesignSystem::layout().px16()
        + Ui::DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const auto bookmarkName = _index.data(BookmarksModel::BookmarkNameRole).toString();
    //
    // ... высота заголовка: отступ сверху + высота иконки + отступ снизу
    //
    const int headerHeight = Ui::DesignSystem::layout().px16()
        + Ui::DesignSystem::treeOneLineItem().iconSize().height()
        + Ui::DesignSystem::layout().px16();
    //
    // ... высота без комментария
    //
    if (bookmarkName.isEmpty()) {
        return { width, headerHeight };
    }
    //
    // ... полная высота
    //
    int height = headerHeight + Ui::DesignSystem::layout().px12()
        + TextHelper::heightForWidth(bookmarkName, Ui::DesignSystem::font().body2(), width);

    return { width, height };
}

} // namespace Ui
