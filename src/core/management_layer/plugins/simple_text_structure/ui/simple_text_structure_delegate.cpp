#include "simple_text_structure_delegate.h"

#include <business_layer/model/simple_text/simple_text_model_chapter_item.h>
#include <business_layer/simple_text_structure_model.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemView>
#include <QPainter>


namespace Ui {

class SimpleTextStructureDelegate::Implementation
{
public:
    /**
     * @brief Нарисовать элемент
     */
    void paintChapter(QPainter* _painter, const QStyleOptionViewItem& _option,
                      const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для элемента
     */
    QSize chapterSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;


    /**
     * @brief Количество строк для отображения под заголовком
     */
    int textLines = 2;
};

void SimpleTextStructureDelegate::Implementation::paintChapter(QPainter* _painter,
                                                               const QStyleOptionViewItem& _option,
                                                               const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = _option.rect;
    if (_option.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = _option.palette.color(QPalette::Highlight);
        textColor = _option.palette.color(QPalette::HighlightedText);
    } else if (_option.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = _option.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... цвет главы
    //
    const auto itemColor = _index.data(TextModelGroupItem::GroupColorRole);
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
            const QRectF itemColorRect(
                isLeftToRight ? 0.0
                              : (backgroundRect.right() + fullIndicatorWidth()
                                 - Ui::DesignSystem::layout().px4()),
                backgroundRect.top(), Ui::DesignSystem::layout().px4(), backgroundRect.height());
            _painter->fillRect(itemColorRect, color);
        }
    }

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        if (isLeftToRight) {
            iconRect = QRectF(
                QPointF(std::max(backgroundRect.left(),
                                 Ui::DesignSystem::treeOneLineItem().margins().left()),
                        backgroundRect.top()),
                QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                       Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24()
                           + Ui::DesignSystem::layout().px16()));
        } else {
            iconRect = QRectF(QPointF(backgroundRect.right()
                                          - Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                      backgroundRect.top()),
                              QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                     Ui::DesignSystem::layout().px16()
                                         + Ui::DesignSystem::layout().px24()
                                         + Ui::DesignSystem::layout().px16()));
        }
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... заголовок главы
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    QRectF headingRect;
    if (isLeftToRight) {
        const qreal headingLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
        const qreal headingWidth = backgroundRect.right()
            - Ui::DesignSystem::treeOneLineItem().margins().right() - headingLeft
            - Ui::DesignSystem::treeOneLineItem().spacing();
        headingRect
            = QRectF(QPointF(headingLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                     QSizeF(headingWidth, Ui::DesignSystem::layout().px24()));
    } else {
        const qreal headingLeft
            = backgroundRect.left() + Ui::DesignSystem::treeOneLineItem().margins().left();
        const qreal headingWidth = iconRect.left() - headingLeft;
        headingRect
            = QRectF(QPointF(headingLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                     QSizeF(headingWidth, Ui::DesignSystem::layout().px24()));
    }
    auto chapterHeading = _index.data(SimpleTextModelChapterItem::GroupHeadingRole).toString();
    chapterHeading = _painter->fontMetrics().elidedText(chapterHeading, Qt::ElideRight,
                                                        static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, chapterHeading);

    //
    // ... текст главы
    //
    auto chapterText = _index.data(SimpleTextModelChapterItem::GroupTextRole).toString();
    if (chapterText.isEmpty()) {
        return;
    }
    QRectF textRect;
    if (textLines > 0) {
        _painter->setFont(Ui::DesignSystem::font().body2());
        if (isLeftToRight) {
            const qreal textLeft = iconRect.left();
            const qreal textWidth = backgroundRect.right() - textLeft
                - Ui::DesignSystem::treeOneLineItem().margins().right();
            textRect
                = QRectF(QPointF(textLeft, headingRect.bottom() + Ui::DesignSystem::layout().px8()),
                         QSizeF(textWidth, _painter->fontMetrics().lineSpacing() * textLines));
        } else {
            const qreal textLeft = headingRect.left();
            const qreal textWidth = iconRect.right() - textLeft;
            textRect
                = QRectF(QPointF(textLeft, headingRect.bottom() + Ui::DesignSystem::layout().px8()),
                         QSizeF(textWidth, _painter->fontMetrics().lineSpacing() * textLines));
        }
        chapterText
            = TextHelper::elidedText(chapterText, Ui::DesignSystem::font().body2(), textRect);
        _painter->drawText(textRect, Qt::TextWordWrap, chapterText);
    }

    //
    // ... иконки заметок
    //
    const auto inlineNotesSize = _index.data(TextModelGroupItem::GroupInlineNotesSizeRole).toInt();
    const qreal notesLeft
        = isLeftToRight ? iconRect.left() : (iconRect.right() - Ui::DesignSystem::layout().px24());
    const qreal notesTop
        = (textRect.isValid() ? textRect : headingRect).bottom() + Ui::DesignSystem::layout().px8();
    const qreal notesHeight = Ui::DesignSystem::layout().px16();
    QRectF inlineNotesRect;
    if (inlineNotesSize > 0) {
        _painter->setFont(Ui::DesignSystem::font().iconsSmall());
        const QRectF inlineNotesIconRect(QPointF(notesLeft, notesTop),
                                         QSizeF(Ui::DesignSystem::layout().px24(), notesHeight));
        _painter->drawText(inlineNotesIconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F09A8");
        //
        _painter->setFont(Ui::DesignSystem::font().caption());
        const auto inlineNotesSizeText = QString::number(inlineNotesSize);
        const auto inlineNotesSizeTextWidth
            = TextHelper::fineTextWidthF(inlineNotesSizeText, _painter->font());
        inlineNotesRect
            = { QPointF(isLeftToRight
                            ? (inlineNotesIconRect.right() + Ui::DesignSystem::layout().px2())
                            : (inlineNotesIconRect.left() - Ui::DesignSystem::layout().px2()
                               - inlineNotesSizeTextWidth),
                        inlineNotesIconRect.top()),
                QSizeF(inlineNotesSizeTextWidth, notesHeight) };
        _painter->drawText(inlineNotesRect, Qt::AlignLeft | Qt::AlignVCenter, inlineNotesSizeText);
    }
    //
    const auto reviewMarksSize = _index.data(TextModelGroupItem::GroupReviewMarksSizeRole).toInt();
    if (reviewMarksSize > 0) {
        _painter->setFont(Ui::DesignSystem::font().iconsSmall());
        const QRectF reviewMarksIconRect(
            QPointF(inlineNotesRect.isValid()
                        ? (isLeftToRight
                               ? (inlineNotesRect.right() + Ui::DesignSystem::layout().px16())
                               : (inlineNotesRect.left() - Ui::DesignSystem::layout().px16()
                                  - Ui::DesignSystem::layout().px24()))
                        : notesLeft,
                    notesTop),
            QSizeF(Ui::DesignSystem::layout().px24(), notesHeight));
        _painter->drawText(reviewMarksIconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F0E31");
        //
        _painter->setFont(Ui::DesignSystem::font().caption());
        const auto reviewMarksSizeText = QString::number(reviewMarksSize);
        const auto reviewMarksSizeTextWidth
            = TextHelper::fineTextWidthF(reviewMarksSizeText, _painter->font());
        const QRectF reviewMarksRect(
            QPointF(isLeftToRight ? (reviewMarksIconRect.right() + Ui::DesignSystem::layout().px2())
                                  : (reviewMarksIconRect.left() - Ui::DesignSystem::layout().px2()
                                     - reviewMarksSizeTextWidth),
                    reviewMarksIconRect.top()),
            QSizeF(reviewMarksSizeTextWidth, notesHeight));
        _painter->drawText(reviewMarksRect, Qt::AlignLeft | Qt::AlignVCenter, reviewMarksSizeText);
    }
}

QSize SimpleTextStructureDelegate::Implementation::chapterSizeHint(
    const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    //
    // Ширина
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= Ui::DesignSystem::layout().px8() + Ui::DesignSystem::layout().px16()
        + Ui::DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const QFontMetricsF fontMetrics(Ui::DesignSystem::font().body2());
    int height = Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24();
    if (textLines > 0) {
        height += Ui::DesignSystem::layout().px8() + fontMetrics.lineSpacing() * textLines
            + Ui::DesignSystem::layout().px16();
    } else {
        height += Ui::DesignSystem::layout().px16();
    }
    const bool haveNotesLine
        = (_index.data(SimpleTextModelChapterItem::GroupInlineNotesSizeRole).toInt()
           + _index.data(SimpleTextModelChapterItem::GroupReviewMarksSizeRole).toInt())
        > 0;
    if (haveNotesLine) {
        height += Ui::DesignSystem::layout().px24();
    }
    return { width, height };
}


// ****


SimpleTextStructureDelegate::SimpleTextStructureDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
    , d(new Implementation)
{
}

SimpleTextStructureDelegate::~SimpleTextStructureDelegate() = default;

void SimpleTextStructureDelegate::setTextLinesSize(int _size)
{
    d->textLines = _size;
}

void SimpleTextStructureDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
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
    d->paintChapter(_painter, opt, _index);
}

QSize SimpleTextStructureDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                            const QModelIndex& _index) const
{
    if (_option.widget == nullptr) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    const auto typeValue = _index.data(Qt::UserRole);
    if (!typeValue.isValid()) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    return d->chapterSizeHint(_option, _index);
}

} // namespace Ui
