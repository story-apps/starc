#include "tree_delegate.h"

#include <ui/design_system/design_system.h>

#include <QPainter>


TreeDelegate::TreeDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

void TreeDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);

    //
    // Определим кисти и шрифты
    //
    QBrush backgroundBrush = Ui::DesignSystem::color().primary();
    QBrush headerBrush = Ui::DesignSystem::color().onPrimary();
    QBrush textBrush = Ui::DesignSystem::color().onPrimary();
    QFont headerFont = opt.font;
//    headerFont.setBold(m_showSceneDescription ? true : false);
    QFont textFont = opt.font;
    textFont.setBold(false);
    QRect backgroundRect = opt.rect;

    //
    // ... фон
    //
    // ... для выделенных элементов
    //
    if (opt.state.testFlag(QStyle::State_Selected)) {
        auto color = Ui::DesignSystem::color().secondary();
        color.setAlphaF(0.5);
        backgroundBrush = color;
        headerBrush = Ui::DesignSystem::color().onSecondary();
        textBrush = Ui::DesignSystem::color().onSecondary();
        backgroundRect.adjust(-28, 6, -6, -6);
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(backgroundBrush);
        _painter->drawRoundedRect(backgroundRect,4, 4);
    }
    //
    // ... для обычных
    //
    else {
        _painter->fillRect(backgroundRect, backgroundBrush);
    }

    //
    // Рисуем
    //

    //
    // Меняем координаты, чтобы рисовать было удобнее
    //
//    _painter->translate(opt.rect.topLeft());

    _painter->setPen(textBrush.color());
    _painter->setFont(textFont);
    _painter->drawText(opt.rect, Qt::AlignLeft | Qt::AlignVCenter, _index.data().toString());

    //
    // ... иконка
    //

//    int iconRectX = QLocale().textDirection() == Qt::LeftToRight
//                    ? 0
//                    : TREE_INDICATOR_WIDTH + opt.rect.width() - m_iconSize - m_itemsHorizontalSpacing - RIGHT_MARGIN;
//    const QRect iconRect(iconRectX, m_iconTopMargin, m_iconSize, m_iconSize);
//    QIcon icon = _index.data(Qt::DecorationRole).value<QIcon>();
//    QColor iconColor = headerBrush.color();
//    // ... если есть заметка, рисуем красноватым цветом
//    if (_index.data(BusinessLogic::ScenarioModel::HasNoteIndex).toBool()) {
//        iconColor = QColor("#ec3838");
//    }
//    ImageHelper::setIconColor(icon, iconRect.size(), iconColor);
//    QPixmap iconPixmap = icon.pixmap(iconRect.size());
//    _painter->drawPixmap(iconRect, iconPixmap);

//    //
//    // ... цвета сцены
//    //
//    const QString colorsNames = _index.data(BusinessLogic::ScenarioModel::ColorIndex).toString();
//    QStringList colorsNamesList = colorsNames.split(";", QString::SkipEmptyParts);
//    int colorsCount = colorsNamesList.size();
//    int colorRectX = QLocale().textDirection() == Qt::LeftToRight
//                     ? TREE_INDICATOR_WIDTH + opt.rect.width() - COLOR_RECT_WIDTH - m_itemsHorizontalSpacing - RIGHT_MARGIN
//                     : 0;
//    if (colorsCount > 0) {
//        //
//        // Если цвет один, то просто рисуем его
//        //
//        if (colorsCount == 1) {
//            const QColor color(colorsNamesList.first());
//            const QRectF colorRect(colorRectX, 0, COLOR_RECT_WIDTH, opt.rect.height());
//            _painter->fillRect(colorRect, color);
//        }
//        //
//        // Если цветов много
//        //
//        else {
//            //
//            // ... первый цвет рисуем на всю вышину в первой колонке
//            //
//            {
//                colorRectX += (QLocale().textDirection() == Qt::LeftToRight ? -1 : 1) * COLOR_RECT_WIDTH;
//                const QColor color(colorsNamesList.takeFirst());
//                const QRectF colorRect(colorRectX, 0, COLOR_RECT_WIDTH, opt.rect.height());
//                _painter->fillRect(colorRect, color);
//            }
//            //
//            // ... остальные цвета рисуем во второй колонке цветов
//            //
//            colorRectX += (QLocale().textDirection() == Qt::LeftToRight ? 1 : -1) * COLOR_RECT_WIDTH;
//            colorsCount = colorsNamesList.size();
//            for (int colorIndex = 0; colorIndex < colorsCount; ++colorIndex) {
//                const QString colorName = colorsNamesList.takeFirst();
//                const QColor color(colorName);
//                const QRectF colorRect(
//                            colorRectX,
//                            opt.rect.height() / qreal(colorsCount) * colorIndex,
//                            COLOR_RECT_WIDTH,
//                            opt.rect.height() / qreal(colorsCount)
//                            );
//                _painter->fillRect(colorRect, color);
//            }
//            //
//            // ... смещаем позицию назад, для корректной отрисовки остального контента
//            //
//            colorRectX += (QLocale().textDirection() == Qt::LeftToRight ? -1 : 1) * COLOR_RECT_WIDTH;
//        }
//    }

//    //
//    // ... текстовая часть
//    //

//    //
//    // ... длительность
//    //
//    _painter->setPen(textBrush.color());
//    _painter->setFont(textFont);
//    const int duration = _index.data(BusinessLogic::ScenarioModel::DurationIndex).toInt();
//    const QString chronometry =
//            BusinessLogic::ChronometerFacade::chronometryUsed()
//            ? "(" + BusinessLogic::ChronometerFacade::secondsToTime(duration)+ ") "
//            : "";
//    const int chronometryRectWidth = _painter->fontMetrics().width(chronometry);
//    const QRect chronometryRect(
//        QLocale().textDirection() == Qt::LeftToRight
//                ? colorRectX - chronometryRectWidth - m_itemsHorizontalSpacing
//                : colorRectX + COLOR_RECT_WIDTH + m_itemsHorizontalSpacing,
//        m_topMargin,
//        chronometryRectWidth,
//        TEXT_LINE_HEIGHT
//        );
//    _painter->drawText(chronometryRect, Qt::AlignLeft | Qt::AlignVCenter, chronometry);

//    //
//    // ... заголовок
//    //
//    _painter->setPen(headerBrush.color());
//    _painter->setFont(headerFont);
//    const QRect headerRect(
//        QLocale().textDirection() == Qt::LeftToRight
//                ? iconRect.right() + m_itemsHorizontalSpacing
//                : chronometryRect.right() + m_itemsHorizontalSpacing,
//        m_topMargin,
//        QLocale().textDirection() == Qt::LeftToRight
//                ? chronometryRect.left() - iconRect.right() - m_itemsHorizontalSpacing*2
//                : iconRect.left() - chronometryRect.right() - m_itemsHorizontalSpacing*2,
//        TEXT_LINE_HEIGHT
//        );
//    QString header = TextEditHelper::smartToUpper(_index.data(Qt::DisplayRole).toString());
//    if (m_showSceneTitle) {
//        //
//        // Если нужно выводим название сцены вместо заголовка
//        //
//        const QString title = TextEditHelper::smartToUpper(_index.data(BusinessLogic::ScenarioModel::TitleIndex).toString());
//        if (!title.isEmpty()) {
//            header = title;
//        }
//    }
//    if (m_showSceneNumber) {
//        //
//        // Если нужно добавляем номер сцены
//        //
//        QVariant sceneNumber = _index.data(BusinessLogic::ScenarioModel::SceneNumberIndex);
//        if (!sceneNumber.isNull()) {
//            header = m_sceneNumbersPrefix + sceneNumber.toString() + ". " + header;
//        }
//    }
//    header = _painter->fontMetrics().elidedText(header, Qt::ElideRight, headerRect.width());
//    _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, header);

//    //
//    // ... описание
//    //
//    if (m_showSceneDescription) {
//        _painter->setPen(textBrush.color());
//        _painter->setFont(textFont);
//        const QRect descriptionRect(
//            QLocale().textDirection() == Qt::LeftToRight
//                    ? headerRect.left()
//                    : chronometryRect.left(),
//            headerRect.bottom() + m_itemsVerticalSpacing,
//            QLocale().textDirection() == Qt::LeftToRight
//                    ? chronometryRect.right() - headerRect.left()
//                    : headerRect.right() - chronometryRect.left(),
//            TEXT_LINE_HEIGHT * m_sceneDescriptionHeight
//            );
//        const QString descriptionText =
//                m_sceneDescriptionIsSceneText
//                ? _index.data(BusinessLogic::ScenarioModel::SceneTextIndex).toString()
//                : _index.data(BusinessLogic::ScenarioModel::DescriptionIndex).toString();
//        _painter->drawText(descriptionRect, Qt::AlignLeft | Qt::TextWordWrap, descriptionText);
//    }

    _painter->restore();
}

QSize TreeDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_option)
    Q_UNUSED(_index)

    return {50, static_cast<int>(Ui::DesignSystem::treeOneLineItem().height())};
}
