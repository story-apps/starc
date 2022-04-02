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
    // ... цвет закладки
    //
    const QRectF colorRect(QPointF(0.0, backgroundRect.top()),
                           QSizeF(Ui::DesignSystem::layout().px4(), backgroundRect.height()));
    _painter->fillRect(colorRect, _index.data(BookmarksModel::BookmarkColorRole).value<QColor>());

    //
    // ... заголовок
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal textLeft = colorRect.right() + Ui::DesignSystem::layout().px16();
    const qreal textWidth = backgroundRect.right() - textLeft - Ui::DesignSystem::layout().px12();
    const QRectF headerRect(
        QPointF(textLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
        QSizeF(textWidth, Ui::DesignSystem::treeOneLineItem().iconSize().height()));
    auto header
        = TextHelper::smartToUpper(_index.data(BookmarksModel::BookmarkItemTextRole).toString());
    header = _painter->fontMetrics().elidedText(header, Qt::ElideRight,
                                                static_cast<int>(headerRect.width()));
    _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, header);

    //
    // ... текст закладки
    //
    const auto bookmarkName = _index.data(BookmarksModel::BookmarkNameRole).toString();
    if (!bookmarkName.isEmpty()) {
        const auto bookmarkNameWidth = backgroundRect.right() - Ui::DesignSystem::layout().px16()
            - Ui::DesignSystem::layout().px16() - Ui::DesignSystem::layout().px8();
        const QRectF bookamrkNameRect(
            QPointF(headerRect.left(), headerRect.bottom() + Ui::DesignSystem::layout().px4()),
            QSizeF(bookmarkNameWidth,
                   TextHelper::heightForWidth(bookmarkName, Ui::DesignSystem::font().body2(),
                                              bookmarkNameWidth)));
        _painter->setFont(Ui::DesignSystem::font().body2());
        _painter->setPen(textColor);
        QTextOption commentTextOption;
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
