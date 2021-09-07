#include "comic_book_text_structure_delegate.h"

#include <business_layer/comic_book_text_structure_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_folder_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_page_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_panel_item.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemView>
#include <QPainter>


namespace Ui {

namespace {
const int kInvalidWordsCount = -1;
}

class ComicBookTextStructureDelegate::Implementation
{
public:
    /**
     * @brief Нарисовать цвет элемента
     */
    void paintItemColor(QPainter* _painter, const QStyleOptionViewItem& _option,
                        const QVariant& _color) const;

    /**
     * @brief Нарисовать количество слов в элемента и получить область где это будет отрисовано
     * @note Если количество не задано, то отрисовка производиться не будет, но область будет
     * рассчитана
     */
    QRectF paintItemWordsCount(QPainter* _painter, const QStyleOptionViewItem& _option,
                               const int _wordsCount = kInvalidWordsCount) const;

    /**
     * @brief Нарисовать элемент
     */
    void paintFolder(QPainter* _painter, const QStyleOptionViewItem& _option,
                     const QModelIndex& _index) const;
    void paintPage(QPainter* _painter, const QStyleOptionViewItem& _option,
                   const QModelIndex& _index) const;
    void paintPanel(QPainter* _painter, const QStyleOptionViewItem& _option,
                    const QModelIndex& _index) const;
    void paintText(QPainter* _painter, const QStyleOptionViewItem& _option,
                   const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для элемент
     */
    QSize folderSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize pageSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize panelSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;


    int textLines = 2;
};

void ComicBookTextStructureDelegate::Implementation::paintItemColor(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QVariant& _color) const
{
    if (_color.isNull() || !_color.canConvert<QColor>()) {
        return;
    }

    const QColor color = _color.value<QColor>();
    if (!color.isValid()) {
        return;
    }

    const auto backgroundRect = _option.rect;
    const QRectF sceneColorRect(0.0, backgroundRect.top(), Ui::DesignSystem::layout().px4(),
                                backgroundRect.height());
    _painter->fillRect(sceneColorRect, color);
}

QRectF ComicBookTextStructureDelegate::Implementation::paintItemWordsCount(
    QPainter* _painter, const QStyleOptionViewItem& _option, const int _wordsCount) const
{
    using namespace BusinessLayer;

    const auto textColor = _option.palette.color(QPalette::Text);
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().body2());

    const auto durationText = QString("(%1)").arg(QString::number(_wordsCount));
    const qreal durationWidth = _painter->fontMetrics().horizontalAdvance(durationText);

    const QRectF backgroundRect = _option.rect;
    const QRectF durationRect(QPointF(backgroundRect.right() - durationWidth
                                          - Ui::DesignSystem::treeOneLineItem().margins().right(),
                                      backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                              QSizeF(durationWidth, Ui::DesignSystem::layout().px24()));
    if (_wordsCount != kInvalidWordsCount) {
        _painter->drawText(durationRect, Qt::AlignLeft | Qt::AlignVCenter, durationText);
    }

    return durationRect;
}

void ComicBookTextStructureDelegate::Implementation::paintFolder(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    // ... цвет папки
    //
    paintItemColor(_painter, _option, _index.data(ComicBookTextModelFolderItem::FolderColorRole));

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        iconRect
            = QRectF(QPointF(std::max(backgroundRect.left(),
                                      Ui::DesignSystem::treeOneLineItem().margins().left()),
                             backgroundRect.top()),
                     QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                            Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24()
                                + Ui::DesignSystem::layout().px16()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... количесвто слов в диалогах
    //
    const auto folderDurationRect = paintItemWordsCount(_painter, _option);

    //
    // ... название папки
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal folderNameLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
    const qreal folderNameWidth = folderDurationRect.left() - folderNameLeft
        - Ui::DesignSystem::treeOneLineItem().spacing();
    const QRectF folderNameRect(
        QPointF(folderNameLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
        QSizeF(folderNameWidth, Ui::DesignSystem::layout().px24()));
    const auto folderName = _painter->fontMetrics().elidedText(
        _index.data(ComicBookTextModelFolderItem::FolderNameRole).toString(), Qt::ElideRight,
        static_cast<int>(folderNameRect.width()));
    _painter->drawText(folderNameRect, Qt::AlignLeft | Qt::AlignVCenter, folderName);
}

void ComicBookTextStructureDelegate::Implementation::paintPage(QPainter* _painter,
                                                               const QStyleOptionViewItem& _option,
                                                               const QModelIndex& _index) const
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
    // ... цвет папки
    //
    paintItemColor(_painter, _option, _index.data(ComicBookTextModelFolderItem::FolderColorRole));

    //
    // ... иконка
    //
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        iconRect
            = QRectF(QPointF(std::max(backgroundRect.left(),
                                      Ui::DesignSystem::treeOneLineItem().margins().left()),
                             backgroundRect.top()),
                     QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                            Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24()
                                + Ui::DesignSystem::layout().px16()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->setPen(_index.data(ComicBookTextModelPageItem::PageHasNumberingErrorRole).toBool()
                             ? DesignSystem::color().error()
                             : textColor);
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... количество слов в диалогах на странице
    //
    const int wordsCount
        = _index.data(ComicBookTextModelPageItem::PageDialoguesWordsCountRole).toInt();
    const auto dialoguesWordsCountRect = paintItemWordsCount(_painter, _option, wordsCount);

    //
    // ... название страницы
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal pageNameLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
    const qreal pageNameWidth = dialoguesWordsCountRect.left() - pageNameLeft
        - Ui::DesignSystem::treeOneLineItem().spacing();
    const QRectF pageNameRect(
        QPointF(pageNameLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
        QSizeF(pageNameWidth, Ui::DesignSystem::layout().px24()));
    const auto pageName = QString("%1 (%2)").arg(
        _index.data(ComicBookTextModelPageItem::PageNameRole).toString(),
        tr("%n PANELS", "", _index.data(ComicBookTextModelPageItem::PagePanelsCountRole).toInt()));
    const auto pageNameCorrected = _painter->fontMetrics().elidedText(
        pageName, Qt::ElideRight, static_cast<int>(pageNameRect.width()));
    _painter->drawText(pageNameRect, Qt::AlignLeft | Qt::AlignVCenter, pageNameCorrected);
}

void ComicBookTextStructureDelegate::Implementation::paintPanel(QPainter* _painter,
                                                                const QStyleOptionViewItem& _option,
                                                                const QModelIndex& _index) const
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
    // ... цвет сцены
    //
    paintItemColor(_painter, _option, _index.data(ComicBookTextModelPanelItem::PanelColorRole));

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        iconRect
            = QRectF(QPointF(std::max(backgroundRect.left(),
                                      Ui::DesignSystem::treeOneLineItem().margins().left()),
                             backgroundRect.top()),
                     QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                            Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px24()
                                + Ui::DesignSystem::layout().px16()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... количество слов в репликах
    //
    const int wordsSize
        = _index.data(ComicBookTextModelPanelItem::PanelDialoguesWordsSizeRole).toInt();
    const auto panelDurationRect = paintItemWordsCount(_painter, _option, wordsSize);

    //
    // ... заголовок сцены
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    const qreal panelHeadingLeft = iconRect.right() + Ui::DesignSystem::layout().px4();
    const qreal panelHeadingWidth = panelDurationRect.left() - panelHeadingLeft
        - Ui::DesignSystem::treeOneLineItem().spacing();
    const QRectF panelHeadingRect(
        QPointF(panelHeadingLeft, backgroundRect.top() + Ui::DesignSystem::layout().px16()),
        QSizeF(panelHeadingWidth, Ui::DesignSystem::layout().px24()));
    auto panelHeading = _index.data(ComicBookTextModelPanelItem::PanelHeadingRole).toString();
    panelHeading = _painter->fontMetrics().elidedText(panelHeading, Qt::ElideRight,
                                                      static_cast<int>(panelHeadingRect.width()));
    _painter->drawText(panelHeadingRect, Qt::AlignLeft | Qt::AlignVCenter, panelHeading);

    //
    // ... текст сцены
    //
    auto panelText = _index.data(ComicBookTextModelPanelItem::PanelTextRole).toString();
    if (panelText.isEmpty()) {
        return;
    }
    QRectF panelTextRect;
    if (textLines > 0) {
        _painter->setFont(Ui::DesignSystem::font().body2());
        const qreal panelTextLeft = iconRect.left();
        const qreal panelTextWidth = backgroundRect.right() - panelTextLeft
            - Ui::DesignSystem::treeOneLineItem().margins().right();
        panelTextRect = QRectF(
            QPointF(panelTextLeft, panelHeadingRect.bottom() + Ui::DesignSystem::layout().px8()),
            QSizeF(panelTextWidth, _painter->fontMetrics().lineSpacing() * textLines));
        panelText
            = TextHelper::elidedText(panelText, Ui::DesignSystem::font().body2(), panelTextRect);
        _painter->drawText(panelTextRect, Qt::TextWordWrap, panelText);
    }

    //
    // ... иконки заметок
    //
    const auto inlineNotesSize
        = _index.data(ComicBookTextModelPanelItem::PanelInlineNotesSizeRole).toInt();
    const qreal notesLeft = iconRect.left();
    const qreal notesTop = (panelTextRect.isValid() ? panelTextRect : panelHeadingRect).bottom()
        + Ui::DesignSystem::layout().px8();
    const qreal notesHeight = Ui::DesignSystem::layout().px16();
    QRectF inlineNotesIconRect;
    if (inlineNotesSize > 0) {
        _painter->setFont(Ui::DesignSystem::font().caption());
        const auto inlineNotesSizeText = QString::number(inlineNotesSize);
        const QRectF inlineNotesRect(
            QPointF(notesLeft, notesTop),
            QSizeF(_painter->fontMetrics().horizontalAdvance(inlineNotesSizeText), notesHeight));
        _painter->drawText(inlineNotesRect, Qt::AlignLeft | Qt::AlignVCenter, inlineNotesSizeText);

        _painter->setFont(Ui::DesignSystem::font().iconsSmall());
        inlineNotesIconRect
            = QRectF(inlineNotesRect.right() + Ui::DesignSystem::layout().px2(),
                     inlineNotesRect.top(), Ui::DesignSystem::layout().px24(), notesHeight);
        _painter->drawText(inlineNotesIconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F09A8");
    }
    const auto reviewMarksSize
        = _index.data(ComicBookTextModelPanelItem::PanelReviewMarksSizeRole).toInt();
    if (reviewMarksSize > 0) {
        _painter->setFont(Ui::DesignSystem::font().caption());
        const auto reviewMarksSizeText = QString::number(reviewMarksSize);
        const QRectF reviewMarksRect(
            QPointF(inlineNotesIconRect.isValid() ? inlineNotesIconRect.right() : notesLeft,
                    notesTop),
            QSizeF(_painter->fontMetrics().horizontalAdvance(reviewMarksSizeText), notesHeight));
        _painter->drawText(reviewMarksRect, Qt::AlignLeft | Qt::AlignVCenter, reviewMarksSizeText);

        _painter->setFont(Ui::DesignSystem::font().iconsSmall());
        const QRectF reviewMarksIconRect(reviewMarksRect.right() + Ui::DesignSystem::layout().px2(),
                                         reviewMarksRect.top(), Ui::DesignSystem::layout().px16(),
                                         notesHeight);
        _painter->drawText(reviewMarksIconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F0E31");
    }
}

QSize ComicBookTextStructureDelegate::Implementation::folderSizeHint(
    const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_index)
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

QSize ComicBookTextStructureDelegate::Implementation::pageSizeHint(
    const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    return folderSizeHint(_option, _index);
}

QSize ComicBookTextStructureDelegate::Implementation::panelSizeHint(
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
        = (_index.data(ComicBookTextModelPanelItem::PanelInlineNotesSizeRole).toInt()
           + _index.data(ComicBookTextModelPanelItem::PanelReviewMarksSizeRole).toInt())
        > 0;
    if (haveNotesLine) {
        height += Ui::DesignSystem::layout().px24();
    }
    return { width, height };
}


// ****


ComicBookTextStructureDelegate::ComicBookTextStructureDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
    , d(new Implementation)
{
}

ComicBookTextStructureDelegate::~ComicBookTextStructureDelegate() = default;

void ComicBookTextStructureDelegate::setTextLinesSize(int _size)
{
    d->textLines = _size;
}

void ComicBookTextStructureDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
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
    const auto type = static_cast<ComicBookTextModelItemType>(typeValue.toInt());
    switch (type) {
    case ComicBookTextModelItemType::Folder: {
        d->paintFolder(_painter, opt, _index);
        break;
    }

    case ComicBookTextModelItemType::Page: {
        d->paintPage(_painter, opt, _index);
        break;
    }

    case ComicBookTextModelItemType::Panel: {
        d->paintPanel(_painter, opt, _index);
        break;
    }

    default:
        break;
    }
}

QSize ComicBookTextStructureDelegate::sizeHint(const QStyleOptionViewItem& _option,
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
    const auto type = static_cast<ComicBookTextModelItemType>(typeValue.toInt());
    switch (type) {
    case ComicBookTextModelItemType::Folder: {
        return d->folderSizeHint(_option, _index);
    }

    case ComicBookTextModelItemType::Page: {
        return d->pageSizeHint(_option, _index);
    }

    case ComicBookTextModelItemType::Panel: {
        return d->panelSizeHint(_option, _index);
    }

    default: {
        return {};
    }
    }
}

} // namespace Ui
