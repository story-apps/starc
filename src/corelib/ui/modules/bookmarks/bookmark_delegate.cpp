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
            const auto backgroundRect = _option.rect;
            itemColorRect = QRectF(
                isLeftToRight ? 0.0 : (backgroundRect.right() - DesignSystem::layout().px4()),
                backgroundRect.top(), DesignSystem::layout().px4(), backgroundRect.height());
            _painter->fillRect(itemColorRect, color);
        }
    }

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (isLeftToRight) {
        iconRect = QRectF(QPointF(std::max(backgroundRect.left(),
                                           DesignSystem::treeOneLineItem().margins().left()),
                                  backgroundRect.top()),
                          QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                                 DesignSystem::treeOneLineItem().height()));
    } else {
        iconRect = QRectF(QPointF(backgroundRect.right()
                                      - DesignSystem::treeOneLineItem().iconSize().width()
                                      - DesignSystem::treeOneLineItem().margins().right(),
                                  backgroundRect.top()),
                          QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                                 DesignSystem::treeOneLineItem().height()));
    }
    _painter->setFont(DesignSystem::font().iconsMid());
    _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F00C0");

    //
    // ... заголовок
    //
    _painter->setFont(DesignSystem::font().subtitle2());
    qreal headingLeft = 0.0;
    qreal headingWidth = 0.0;
    if (isLeftToRight) {
        headingLeft = iconRect.right() + DesignSystem::treeOneLineItem().spacing();
        headingWidth
            = backgroundRect.right() - headingLeft - DesignSystem::treeOneLineItem().spacing();
    } else {
        headingLeft = DesignSystem::treeOneLineItem().spacing();
        headingWidth = iconRect.left() - headingLeft - DesignSystem::treeOneLineItem().spacing();
    }
    const QRectF headingRect(
        QPointF(headingLeft,
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        QSizeF(headingWidth, DesignSystem::layout().px24()));
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
        const qreal textLeft = isLeftToRight
            ? iconRect.left()
            : backgroundRect.left() + DesignSystem::treeOneLineItem().margins().left();
        const qreal textWidth = isLeftToRight
            ? backgroundRect.width() - iconRect.left()
                - DesignSystem::treeOneLineItem().margins().right()
            : iconRect.right() - DesignSystem::treeOneLineItem().margins().left();
        const QRectF bookamrkNameRect(
            QPointF(textLeft, headingRect.bottom() + DesignSystem::compactLayout().px8()),
            QSizeF(
                textWidth,
                TextHelper::heightForWidth(bookmarkName, DesignSystem::font().body2(), textWidth)));

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
