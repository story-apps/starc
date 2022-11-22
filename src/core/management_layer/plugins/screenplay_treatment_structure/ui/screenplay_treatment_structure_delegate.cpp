#include "screenplay_treatment_structure_delegate.h"

#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/screenplay_treatment_structure_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QAbstractItemView>
#include <QPainter>


namespace Ui {

class ScreenplayTreatmentStructureDelegate::Implementation
{
public:
    /**
     * @brief Нарисовать цвет элемента
     */
    void paintItemColor(QPainter* _painter, const QStyleOptionViewItem& _option,
                        const QVariant& _color, const QModelIndex& _index) const;

    /**
     * @brief Нарисовать элемент
     */
    void paintFolder(QPainter* _painter, const QStyleOptionViewItem& _option,
                     const QModelIndex& _index) const;
    void paintScene(QPainter* _painter, const QStyleOptionViewItem& _option,
                    const QModelIndex& _index) const;
    void paintBeat(QPainter* _painter, const QStyleOptionViewItem& _option,
                   const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для элемента
     */
    QSize folderSizeHint(const QStyleOptionViewItem& _option) const;
    QSize sceneSizeHint(const QStyleOptionViewItem& _option) const;
    QSize beatSizeHint(const QStyleOptionViewItem& _option) const;


    bool showSceneNumber = true;
    int textLines = 0;
};

void ScreenplayTreatmentStructureDelegate::Implementation::paintItemColor(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QVariant& _color,
    const QModelIndex& _index) const
{
    if (_color.isNull() || !_color.canConvert<QColor>()) {
        return;
    }

    const QColor color = _color.value<QColor>();
    if (!color.isValid()) {
        return;
    }

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
    const QRectF colorRect(
        QLocale().textDirection() == Qt::LeftToRight
            ? 0.0
            : (backgroundRect.right() + fullIndicatorWidth() - Ui::DesignSystem::layout().px4()),
        backgroundRect.top(), Ui::DesignSystem::layout().px4(), backgroundRect.height());
    _painter->fillRect(colorRect, color);
}

void ScreenplayTreatmentStructureDelegate::Implementation::paintFolder(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    // ... цвет папки
    //
    paintItemColor(_painter, _option, _index.data(TextModelFolderItem::FolderColorRole), _index);

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
    // ... название папки
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
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
    const auto folderName = _painter->fontMetrics().elidedText(
        _index.data(TextModelFolderItem::FolderHeadingRole).toString(), Qt::ElideRight,
        static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, folderName);
}

void ScreenplayTreatmentStructureDelegate::Implementation::paintScene(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    // ... цвет сцены
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
    // ... заголовок сцены
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
    auto sceneHeading
        = TextHelper::smartToUpper(_index.data(TextModelGroupItem::GroupHeadingRole).toString());
    if (showSceneNumber) {
        sceneHeading.prepend(_index.data(TextModelGroupItem::GroupNumberRole).toString() + " ");
    }
    sceneHeading = _painter->fontMetrics().elidedText(sceneHeading, Qt::ElideRight,
                                                      static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, sceneHeading);
}

void ScreenplayTreatmentStructureDelegate::Implementation::paintBeat(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    // ... цвет бита
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
    // ... текст элемента
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
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
    const auto heading = _painter->fontMetrics().elidedText(
        TextHelper::smartToUpper(_index.data(TextModelGroupItem::GroupHeadingRole).toString()),
        Qt::ElideRight, static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, heading);
}

QSize ScreenplayTreatmentStructureDelegate::Implementation::folderSizeHint(
    const QStyleOptionViewItem& _option) const
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
    int height = Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24()
        + Ui::DesignSystem::layout().px16();

    return { width, height };
}

QSize ScreenplayTreatmentStructureDelegate::Implementation::sceneSizeHint(
    const QStyleOptionViewItem& _option) const
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
    int height = Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24();
    if (textLines > 0) {
        height += Ui::DesignSystem::layout().px8()
            + TextHelper::fineLineSpacing(Ui::DesignSystem::font().body2()) * textLines
            + Ui::DesignSystem::layout().px16();
    } else {
        height += Ui::DesignSystem::layout().px16();
    }
    return { width, height };
}

QSize ScreenplayTreatmentStructureDelegate::Implementation::beatSizeHint(
    const QStyleOptionViewItem& _option) const
{
    return folderSizeHint(_option);
}


// ****


ScreenplayTreatmentStructureDelegate::ScreenplayTreatmentStructureDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
    , d(new Implementation)
{
}

ScreenplayTreatmentStructureDelegate::~ScreenplayTreatmentStructureDelegate() = default;

void ScreenplayTreatmentStructureDelegate::setShowFullBeatText(bool _showFull)
{
}

void ScreenplayTreatmentStructureDelegate::showSceneNumber(bool _show)
{
    d->showSceneNumber = _show;
}

void ScreenplayTreatmentStructureDelegate::setTextLinesSize(int _size)
{
    d->textLines = _size;
}

void ScreenplayTreatmentStructureDelegate::paint(QPainter* _painter,
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

    //
    // Рисуем в зависимости от типа элемента
    //

    const auto typeValue = _index.data(Qt::UserRole);
    if (!typeValue.isValid()) {
        return;
    }

    using namespace BusinessLayer;
    const auto type = static_cast<TextModelItemType>(typeValue.toInt());
    switch (type) {
    case TextModelItemType::Folder: {
        d->paintFolder(_painter, opt, _index);
        break;
    }

    case TextModelItemType::Group: {
        const auto groupType
            = static_cast<TextGroupType>(_index.data(TextModelGroupItem::GroupTypeRole).toInt());
        if (groupType == TextGroupType::Scene) {
            d->paintScene(_painter, opt, _index);
        } else {
            d->paintBeat(_painter, opt, _index);
        }
        break;
    }

    default:
        break;
    }
}

QSize ScreenplayTreatmentStructureDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                                     const QModelIndex& _index) const
{
    if (_option.widget == nullptr) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    const auto typeValue = _index.data(Qt::UserRole);
    if (!typeValue.isValid()) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    using namespace BusinessLayer;
    const auto type = static_cast<TextModelItemType>(typeValue.toInt());
    switch (type) {
    case TextModelItemType::Folder: {
        return d->folderSizeHint(_option);
    }

    case TextModelItemType::Group: {
        const auto groupType
            = static_cast<TextGroupType>(_index.data(TextModelGroupItem::GroupTypeRole).toInt());
        if (groupType == TextGroupType::Scene) {
            return d->sceneSizeHint(_option);
        } else {
            return d->beatSizeHint(_option);
        }
    }

    default: {
        return {};
    }
    }
}

} // namespace Ui
