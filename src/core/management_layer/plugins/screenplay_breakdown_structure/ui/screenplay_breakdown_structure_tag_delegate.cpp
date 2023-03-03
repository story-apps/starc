#include "screenplay_breakdown_structure_tag_delegate.h"

#include "business_layer/screenplay_breakdown_structure_model_item.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QAbstractItemView>
#include <QPainter>


namespace Ui {

class ScreenplayBreakdownStructureTagDelegate::Implementation
{
public:
    /**
     * @brief Нарисовать цвет элемента
     */
    void paintItemColor(QPainter* _painter, const QStyleOptionViewItem& _option,
                        const QVariant& _color, const QModelIndex& _index) const;

    /**
     * @brief Нарисовать хронометраж
     */
    QRectF paintItemDuration(QPainter* _painter, const QStyleOptionViewItem& _option,
                             const std::chrono::seconds& _duration) const;

    /**
     * @brief Нарисовать элемент
     */
    void paintItem(QPainter* _painter, const QStyleOptionViewItem& _option,
                   const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для элемент
     */
    QSize itemSizeHint(const QStyleOptionViewItem& _option) const;
};

void ScreenplayBreakdownStructureTagDelegate::Implementation::paintItemColor(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QVariant& _color,
    const QModelIndex& _index) const
{
    //
    // Собираем цвета до самого верха
    //
    QVector<QColor> colors;
    auto addColor = [&colors](const QVariant& _color) {
        if (_color.isNull() || !_color.canConvert<QColor>()) {
            return;
        }

        const QColor color = _color.value<QColor>();
        if (!color.isValid()) {
            return;
        }

        colors.prepend(color);
    };
    addColor(_color);
    //
    auto index = _index;
    while (index.parent().isValid()) {
        index = index.parent();

        const auto typeValue = index.data(Qt::UserRole);
        if (!typeValue.isValid()) {
            continue;
        }

        using namespace BusinessLayer;
        const auto type = static_cast<TextModelItemType>(typeValue.toInt());
        switch (type) {
        case TextModelItemType::Folder: {
            addColor(index.data(TextModelFolderItem::FolderColorRole));
            break;
        }
        case TextModelItemType::Group: {
            addColor(index.data(TextModelGroupItem::GroupColorRole));
            break;
        }
        default: {
            break;
        }
        }
    }

    //
    // Рисуем цвета
    //
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
    auto lastX = QLocale().textDirection() == Qt::LeftToRight
        ? 0.0
        : (backgroundRect.right() + fullIndicatorWidth() - Ui::DesignSystem::layout().px(5));
    for (const auto& color : colors) {
        const QRectF colorRect(lastX, backgroundRect.top(), Ui::DesignSystem::layout().px(5),
                               backgroundRect.height());
        _painter->fillRect(colorRect, color);

        lastX += (QLocale().textDirection() == Qt::LeftToRight ? 1 : -1)
            * Ui::DesignSystem::layout().px(7);
    }
}

QRectF ScreenplayBreakdownStructureTagDelegate::Implementation::paintItemDuration(
    QPainter* _painter, const QStyleOptionViewItem& _option,
    const std::chrono::seconds& _duration) const
{
    using namespace BusinessLayer;

    _painter->setFont(Ui::DesignSystem::font().body2());

    const auto durationText = QString("(%1)").arg(TimeHelper::toString(_duration));
    const qreal durationWidth = TextHelper::fineTextWidthF(durationText, _painter->font());

    const QRectF backgroundRect = _option.rect;
    const QRectF durationRect(
        QPointF(QLocale().textDirection() == Qt::LeftToRight
                    ? (backgroundRect.right() - durationWidth
                       - Ui::DesignSystem::treeOneLineItem().margins().right())
                    : backgroundRect.left() + Ui::DesignSystem::treeOneLineItem().margins().left(),
                backgroundRect.top() + Ui::DesignSystem::layout().px16()),
        QSizeF(durationWidth, Ui::DesignSystem::layout().px24()));
    _painter->drawText(durationRect, Qt::AlignLeft | Qt::AlignVCenter, durationText);

    return durationRect;
}

void ScreenplayBreakdownStructureTagDelegate::Implementation::paintItem(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const auto isHighlighted
        = _index.data(ScreenplayBreakdownStructureModelItem::HighlightedRole).toBool();

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
        if (!isHighlighted) {
            textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
        }
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... цвет папки
    //
    paintItemColor(_painter, _option, _index.data(TextModelGroupItem::GroupColorRole), _index);

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
    // ... хронометраж
    //
    const std::chrono::seconds duration{
        _index.data(ScreenplayBreakdownStructureModelItem::DurationRole).toInt()
    };
    const auto durationRect = paintItemDuration(_painter, _option, duration);

    //
    // ... название папки
    //
    auto textFont = Ui::DesignSystem::font().subtitle2();
    textFont.setWeight(isHighlighted ? QFont::Weight::Bold : QFont::Normal);
    _painter->setFont(textFont);
    _painter->setPen(textColor);
    QRectF headingRect;
    if (isLeftToRight) {
        const qreal hadingLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
        const qreal headingWidth
            = durationRect.left() - hadingLeft - Ui::DesignSystem::treeOneLineItem().spacing();
        headingRect
            = QRectF(QPointF(hadingLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                     QSizeF(headingWidth, Ui::DesignSystem::layout().px24()));
    } else {
        const qreal hadingLeft
            = durationRect.right() + Ui::DesignSystem::treeOneLineItem().spacing();
        const qreal headingWidth = iconRect.left() - hadingLeft - Ui::DesignSystem::layout().px4();
        headingRect
            = QRectF(QPointF(hadingLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                     QSizeF(headingWidth, Ui::DesignSystem::layout().px24()));
    }
    const auto folderName
        = _painter->fontMetrics().elidedText(_index.data(Qt::DisplayRole).toString(),
                                             Qt::ElideRight, static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, folderName);
}

QSize ScreenplayBreakdownStructureTagDelegate::Implementation::itemSizeHint(
    const QStyleOptionViewItem& _option) const
{
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
    int height = Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24()
        + Ui::DesignSystem::layout().px16();

    return { width, height };
}


// ****


ScreenplayBreakdownStructureTagDelegate::ScreenplayBreakdownStructureTagDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
    , d(new Implementation)
{
}

ScreenplayBreakdownStructureTagDelegate::~ScreenplayBreakdownStructureTagDelegate() = default;


void ScreenplayBreakdownStructureTagDelegate::paint(QPainter* _painter,
                                                    const QStyleOptionViewItem& _option,
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
    d->paintItem(_painter, opt, _index);
}

QSize ScreenplayBreakdownStructureTagDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                                        const QModelIndex& _index) const
{
    if (_option.widget == nullptr) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    return d->itemSizeHint(_option);
}

} // namespace Ui
