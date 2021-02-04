#include "screenplay_text_structure_delegate.h"

#include <business_layer/screenplay_text_structure_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QAbstractItemView>
#include <QPainter>


namespace Ui
{

class ScreenplayTextStructureDelegate::Implementation
{
public:
    /**
     * @brief Нарисовать хронометраж
     */
    QRectF paintDuration(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

    /**
     * @brief Нарисовать папку
     */
    void paintFolder(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

    /**
     * @brief Нарисовать сцену
     */
    void paintScene(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для папки
     */
    QSize folderSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для сцены
     */
    QSize sceneSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;


    bool showSceneNumber = true;
    int textLines = 2;
};

QRectF ScreenplayTextStructureDelegate::Implementation::paintDuration(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    const auto textColor = _option.palette.color(QPalette::Text);
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().body2());

    const std::chrono::seconds duration{_index.data(ScreenplayTextModelSceneItem::SceneDurationRole).toInt()};
    const auto durationText = QString("(%1)").arg(TimeHelper::toString(duration));
    const qreal durationWidth = _painter->fontMetrics().horizontalAdvance(durationText);

    const QRectF backgroundRect = _option.rect;
    const QRectF durationRect(QPointF(backgroundRect.right() - durationWidth - Ui::DesignSystem::treeOneLineItem().margins().right(),
                                           backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                                   QSizeF(durationWidth,
                                          Ui::DesignSystem::layout().px24()));
    _painter->drawText(durationRect, Qt::AlignLeft | Qt::AlignVCenter, durationText);

    return durationRect;
}

void ScreenplayTextStructureDelegate::Implementation::paintFolder(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);

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
    // ... TODO: цвет папки
    //

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
                                 Ui::DesignSystem::layout().px16()
                                 + Ui::DesignSystem::layout().px24()
                                 + Ui::DesignSystem::layout().px16()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter, _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... хронометраж
    //
    const auto folderDurationRect = paintDuration(_painter, _option, _index);

    //
    // ... название папки
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal folderNameLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
    const qreal folderNameWidth = folderDurationRect.left() - folderNameLeft - Ui::DesignSystem::treeOneLineItem().spacing();
    const QRectF folderNameRect(QPointF(folderNameLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                                QSizeF(folderNameWidth, Ui::DesignSystem::layout().px24()));
    const auto folderName = _painter->fontMetrics().elidedText(
                                _index.data(ScreenplayTextModelFolderItem::FolderNameRole).toString(),
                                Qt::ElideRight,
                                static_cast<int>(folderNameRect.width()));
    _painter->drawText(folderNameRect, Qt::AlignLeft | Qt::AlignVCenter, folderName);
}

void ScreenplayTextStructureDelegate::Implementation::paintScene(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);

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
    // ... TODO: цвет сцены
    //

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
                                 Ui::DesignSystem::layout().px16()
                                 + Ui::DesignSystem::layout().px24()
                                 + Ui::DesignSystem::layout().px16()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter, _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... хронометраж
    //
    const auto sceneDurationRect = paintDuration(_painter, _option, _index);

    //
    // ... заголовок сцены
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    const qreal sceneHeadingLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
    const qreal sceneHeadingWidth = sceneDurationRect.left() - sceneHeadingLeft - Ui::DesignSystem::treeOneLineItem().spacing();
    const QRectF sceneHeadingRect(QPointF(sceneHeadingLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                                  QSizeF(sceneHeadingWidth, Ui::DesignSystem::layout().px24()));
    auto sceneHeading = _index.data(ScreenplayTextModelSceneItem::SceneHeadingRole).toString();
    if (showSceneNumber) {
        sceneHeading.prepend(_index.data(ScreenplayTextModelSceneItem::SceneNumberRole).toString() + " ");
    }
    sceneHeading = _painter->fontMetrics().elidedText(
                       sceneHeading,
                       Qt::ElideRight,
                       static_cast<int>(sceneHeadingRect.width()));
    _painter->drawText(sceneHeadingRect, Qt::AlignLeft | Qt::AlignVCenter, sceneHeading);

    //
    // ... текст сцены
    //
    auto sceneText = _index.data(ScreenplayTextModelSceneItem::SceneTextRole).toString();
    if (sceneText.isEmpty()) {
        return;
    }
    QRectF sceneTextRect;
    if (textLines > 0) {
        _painter->setFont(Ui::DesignSystem::font().body2());
        const qreal sceneTextLeft = iconRect.left();
        const qreal sceneTextWidth = backgroundRect.right() - sceneTextLeft - Ui::DesignSystem::treeOneLineItem().margins().right();
        sceneTextRect = QRectF(QPointF(sceneTextLeft, sceneHeadingRect.bottom() + Ui::DesignSystem::layout().px8()),
                               QSizeF(sceneTextWidth, _painter->fontMetrics().lineSpacing() * textLines));
        sceneText = TextHelper::elidedText(sceneText,
                                           Ui::DesignSystem::font().body2(),
                                           sceneTextRect);
        _painter->drawText(sceneTextRect, Qt::TextWordWrap, sceneText);
    }

    //
    // ... иконки заметок
    //
    const auto inlineNotesSize = _index.data(ScreenplayTextModelSceneItem::SceneInlineNotesSizeRole).toInt();
    const qreal notesLeft = iconRect.left();
    const qreal notesTop = (sceneTextRect.isValid() ? sceneTextRect : sceneHeadingRect).bottom()
                           + Ui::DesignSystem::layout().px8();
    const qreal notesHeight = Ui::DesignSystem::layout().px16();
    QRectF inlineNotesIconRect;
    if (inlineNotesSize > 0) {
        _painter->setFont(Ui::DesignSystem::font().caption());
        const auto inlineNotesSizeText = QString::number(inlineNotesSize);
        const QRectF inlineNotesRect(QPointF(notesLeft, notesTop),
                                     QSizeF(_painter->fontMetrics().horizontalAdvance(inlineNotesSizeText),
                                            notesHeight));
        _painter->drawText(inlineNotesRect, Qt::AlignLeft | Qt::AlignVCenter, inlineNotesSizeText);

        _painter->setFont(Ui::DesignSystem::font().iconsSmall());
        inlineNotesIconRect = QRectF(inlineNotesRect.right() + Ui::DesignSystem::layout().px2(),
                                     inlineNotesRect.top(),
                                     Ui::DesignSystem::layout().px24(),
                                     notesHeight);
        _painter->drawText(inlineNotesIconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F09A8");
    }
    const auto reviewMarksSize = _index.data(ScreenplayTextModelSceneItem::SceneReviewMarksSizeRole).toInt();
    if (reviewMarksSize > 0) {
        _painter->setFont(Ui::DesignSystem::font().caption());
        const auto reviewMarksSizeText = QString::number(reviewMarksSize);
        const QRectF reviewMarksRect(QPointF(inlineNotesIconRect.isValid() ? inlineNotesIconRect.right()
                                                                           : notesLeft,
                                             notesTop),
                                     QSizeF(_painter->fontMetrics().horizontalAdvance(reviewMarksSizeText),
                                            notesHeight));
        _painter->drawText(reviewMarksRect, Qt::AlignLeft | Qt::AlignVCenter, reviewMarksSizeText);

        _painter->setFont(Ui::DesignSystem::font().iconsSmall());
        const QRectF reviewMarksIconRect(reviewMarksRect.right() + Ui::DesignSystem::layout().px2(),
                                         reviewMarksRect.top(),
                                         Ui::DesignSystem::layout().px16(),
                                         notesHeight);
        _painter->drawText(reviewMarksIconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F0E31");
    }
}

QSize ScreenplayTextStructureDelegate::Implementation::folderSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    //
    // Ширина
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= Ui::DesignSystem::layout().px8()
             + Ui::DesignSystem::layout().px16()
             + Ui::DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const QFontMetricsF fontMetrics(Ui::DesignSystem::font().body2());
    int height = Ui::DesignSystem::layout().px16()
                 + Ui::DesignSystem::layout().px24()
                 + Ui::DesignSystem::layout().px16();

    return { width, height };
}

QSize ScreenplayTextStructureDelegate::Implementation::sceneSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    //
    // Ширина
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= Ui::DesignSystem::layout().px8()
             + Ui::DesignSystem::layout().px16()
             + Ui::DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const QFontMetricsF fontMetrics(Ui::DesignSystem::font().body2());
    int height = Ui::DesignSystem::layout().px16()
                 + Ui::DesignSystem::layout().px24();
    if (textLines > 0) {
        height += Ui::DesignSystem::layout().px8()
                  + fontMetrics.lineSpacing() * textLines
                  + Ui::DesignSystem::layout().px16();
    } else {
        height += Ui::DesignSystem::layout().px16();
    }
    const bool haveNotesLine
            = (_index.data(ScreenplayTextModelSceneItem::SceneInlineNotesSizeRole).toInt()
               + _index.data(ScreenplayTextModelSceneItem::SceneReviewMarksSizeRole).toInt())
              > 0;
    if (haveNotesLine) {
        height += Ui::DesignSystem::layout().px24();
    }
    return { width, height };
}


// ****


ScreenplayTextStructureDelegate::ScreenplayTextStructureDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent),
      d(new Implementation)
{
}

ScreenplayTextStructureDelegate::~ScreenplayTextStructureDelegate() = default;

void ScreenplayTextStructureDelegate::showSceneNumber(bool _show)
{
    d->showSceneNumber = _show;
}

void ScreenplayTextStructureDelegate::setTextLinesSize(int _size)
{
    d->textLines = _size;
}

void ScreenplayTextStructureDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    const auto type = static_cast<ScreenplayTextModelItemType>(typeValue.toInt());
    switch (type) {
        case ScreenplayTextModelItemType::Folder: {
            d->paintFolder(_painter, opt, _index);
            break;
        }

        case ScreenplayTextModelItemType::Scene: {
            d->paintScene(_painter, opt, _index);
            break;
        }

        default: break;
    }
}

QSize ScreenplayTextStructureDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    if (_option.widget == nullptr) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    const auto typeValue = _index.data(Qt::UserRole);
    if (!typeValue.isValid()) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    using namespace BusinessLayer;
    const auto type = static_cast<ScreenplayTextModelItemType>(typeValue.toInt());
    switch (type) {
        case ScreenplayTextModelItemType::Folder: {
            return d->folderSizeHint(_option, _index);
        }

        case ScreenplayTextModelItemType::Scene: {
            return d->sceneSizeHint(_option, _index);
        }

        default: {
            return {};
        }
    }
}

} // namespace Ui
