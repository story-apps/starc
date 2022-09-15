#include "color_palette.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QMouseEvent>
#include <QPainter>
#include <QSettings>


namespace {
const QString kColorsKey = QLatin1String("widgets/color-picker/colors");
const QString kColorsSeparator = QLatin1String(";");
} // namespace


class ColorPallete::Implementation
{
public:
    /**
     * @brief Сформировать палитру
     */
    void buildPalette();

    /**
     * @brief Проверить, есть ли такой цвет в палитре
     */
    bool hasColor(const QColor& _color) const;


    struct ColorItem {
        bool operator==(const ColorItem& _other) const
        {
            return color == _other.color && rect == _other.rect;
        }

        QColor color = QColor::Invalid;
        QRectF rect;
    };
    QVector<ColorItem> colorsPalette;
    ColorItem selectedColor;
    QVector<QColor> customColors;
    QRectF addCustomColorRect;

    bool isColorCanBeDeslected = false;
};


void ColorPallete::Implementation::buildPalette()
{
    colorsPalette.clear();

    const int colorRectColumns = 10;
    const QSizeF colorRectSize
        = { Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24() };
    const qreal colorRectSpace = Ui::DesignSystem::layout().px4();

    //
    // Формируем первый ряд
    //
    int topMargin = Ui::DesignSystem::layout().px24();
    int leftMargin = Ui::DesignSystem::layout().px12();
    QList<QColor> colors;
    colors << QColor("#000000") << QColor("#434343") << QColor("#666666") << QColor("#999999")
           << QColor("#B7B7B7") << QColor("#CCCCCC") << QColor("#D9D9D9") << QColor("#EFEFEF")
           << QColor("#F3F3F3") << QColor("#FFFFFF");
    for (int column = 0; column < colorRectColumns; ++column) {
        QRectF colorRect;
        colorRect.setLeft(leftMargin);
        colorRect.setTop(topMargin);
        colorRect.setSize(colorRectSize);

        colorsPalette.append({ colors.at(column), colorRect });

        leftMargin += colorRectSize.width() + colorRectSpace;
    }
    topMargin += colorRectSize.height() + Ui::DesignSystem::layout().px48();


    //
    // Остальные ряды
    //
    colors.clear();
    colors << QColor("#FE0000") << QColor("#FF7401") << QColor("#FFD302") << QColor("#00FF00")
           << QColor("#01CC01") << QColor("#06E3E4") << QColor("#0046F4") << QColor("#4F18FF")
           << QColor("#9706E7") << QColor("#FF00A5") // ****
           << QColor("#FE3235") << QColor("#FF9036") << QColor("#FFDB34") << QColor("#A3FA14")
           << QColor("#35D533") << QColor("#44D0D1") << QColor("#4174F0") << QColor("#653EE0")
           << QColor("#A048CF") << QColor("#F22B9E") // ****
           << QColor("#FF686A") << QColor("#FFAC66") << QColor("#FCE364") << QColor("#C6F567")
           << QColor("#65DF66") << QColor("#77DDE0") << QColor("#7293F4") << QColor("#8964EF")
           << QColor("#B74EED") << QColor("#F35EB2") // ****
           << QColor("#FF999C") << QColor("#FFC79C") << QColor("#FFEC99") << QColor("#DAF798")
           << QColor("#99EB99") << QColor("#B2F0F1") << QColor("#94B3F6") << QColor("#B29BF5")
           << QColor("#CF92F4") << QColor("#FE8ACA") // ****
           << QColor("#F4CCCC") << QColor("#FCE5CD") << QColor("#FFF2CC") << QColor("#D9EAD3")
           << QColor("#CAFFCA") << QColor("#BEFEFF") << QColor("#B2CEFF") << QColor("#CFBDF8")
           << QColor("#E3B6FF") << QColor("#FFAADA");
    const int colorRectRows = colors.size() / 10;
    for (int row = 0; row < colorRectRows; ++row) {
        leftMargin = Ui::DesignSystem::layout().px12();
        for (int column = 0; column < colorRectColumns; ++column) {
            QRectF colorRect;
            colorRect.setLeft(leftMargin);
            colorRect.setTop(topMargin);
            colorRect.setSize(colorRectSize);

            colorsPalette.append({ colors.at((row * colorRectColumns) + column), colorRect });

            leftMargin += colorRectSize.width() + colorRectSpace;
        }
        topMargin += colorRectSize.height() + colorRectSpace;
    }

    topMargin += -colorRectSpace + Ui::DesignSystem::layout().px62();
    leftMargin = Ui::DesignSystem::layout().px12();
    for (const auto& color : std::as_const(customColors)) {
        QRectF colorRect;
        colorRect.setLeft(leftMargin);
        colorRect.setTop(topMargin);
        colorRect.setSize(colorRectSize);

        colorsPalette.append({ color, colorRect });

        leftMargin += colorRectSize.width() + colorRectSpace;
    }

    addCustomColorRect.setLeft(leftMargin);
    addCustomColorRect.setTop(topMargin);
    addCustomColorRect.setSize(colorRectSize);
}

bool ColorPallete::Implementation::hasColor(const QColor& _color) const
{
    for (const auto& color : std::as_const(colorsPalette)) {
        if (color.color == _color) {
            return true;
        }
    }

    return false;
}


// ****


ColorPallete::ColorPallete(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setMouseTracking(true);

    const auto customColors
        = QSettings().value(kColorsKey).toString().split(kColorsSeparator, Qt::SkipEmptyParts);
    for (const auto& color : customColors) {
        d->customColors.append(color);
    }
}

ColorPallete::~ColorPallete() = default;

void ColorPallete::setColorCanBeDeselected(bool _can)
{
    d->isColorCanBeDeslected = _can;
}

QColor ColorPallete::selectedColor() const
{
    return d->selectedColor.color;
}

void ColorPallete::setSelectedColor(const QColor& _color)
{
    if (d->selectedColor.color == _color) {
        return;
    }

    if (!_color.isValid()) {
        d->selectedColor = {};
    } else {
        if (!d->hasColor(_color)) {
            addCustomColor(_color);
        }

        for (const auto& color : std::as_const(d->colorsPalette)) {
            if (color.color != _color) {
                continue;
            }
            d->selectedColor = color;
            break;
        }
    }

    update();
}

void ColorPallete::addCustomColor(const QColor& _color)
{
    //
    // Если такой цвет уже есть, переместим его в конец
    //
    if (d->customColors.contains(_color)) {
        d->customColors.move(d->customColors.indexOf(_color), d->customColors.size() - 1);
    }
    //
    // Если же цвета не было, то добавим его в пределах допустимой нормы цветов
    //
    else {
        const int maxColorsSize = 9;
        if (d->customColors.size() == maxColorsSize) {
            d->customColors.removeFirst();
        }
        d->customColors.append(_color);
    }

    //
    // Сохраним цвета
    //
    const QString colorsValue = [colors = d->customColors] {
        QString colorsText;
        for (const auto& color : colors) {
            colorsText.append(color.name() + kColorsSeparator);
        }
        return colorsText;
    }();
    QSettings().setValue(kColorsKey, colorsValue);

    //
    // Обновим внешний вид
    //
    d->buildPalette();
    update();
}

QSize ColorPallete::sizeHint() const
{
    return QSize(d->colorsPalette.value(9).rect.right() + Ui::DesignSystem::layout().px12(),
                 d->addCustomColorRect.bottom() + Ui::DesignSystem::layout().px12());
}

void ColorPallete::paintEvent(QPaintEvent* _event)
{
    Widget::paintEvent(_event);

    Q_UNUSED(_event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем кружки с цветами
    //
    const QColor smoothTextColor
        = ColorHelper::transparent(textColor(), Ui::DesignSystem::hoverBackgroundOpacity());
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    for (const auto& color : std::as_const(d->colorsPalette)) {
        //
        // Сам цвет
        //
        painter.setPen(smoothTextColor);
        painter.setBrush(color.color);
        painter.drawEllipse(color.rect);

        //
        // Текущий
        //
        if (color == d->selectedColor) {
            painter.setPen(ColorHelper::contrasted(color.color));
            painter.setFont(Ui::DesignSystem::font().iconsSmall());
            painter.drawText(color.rect, Qt::AlignCenter,
                             d->isColorCanBeDeslected && color.rect.contains(mousePos)
                                 ? u8"\U000F0156"
                                 : u8"\U000F012C");
        }

        //
        // Под мышкой
        //
        if (color.rect.contains(mousePos)) {
            const auto adjustSize = Ui::DesignSystem::layout().px2();
            const auto borderRect
                = color.rect.adjusted(-adjustSize, -adjustSize, adjustSize, adjustSize);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(color.color, 1.0 * Ui::DesignSystem::scaleFactor()));
            painter.drawEllipse(borderRect);
        }
    }

    //
    // Разделитель между верхней и центральной
    //
    const QRectF dividerRect(0, Ui::DesignSystem::layout().px(72), width(),
                             Ui::DesignSystem::scaleFactor());
    painter.fillRect(dividerRect, smoothTextColor);

    //
    // Кастомные цвета
    //
    // ... заголовок
    //
    painter.setPen(QPen(textColor(), Ui::DesignSystem::layout().px2()));
    painter.setBrush(Qt::NoBrush);
    painter.setFont(Ui::DesignSystem::font().button());
    const qreal otherColorsLabelTop
        = (24 + 24 + 24 + 24 + 24 * 5 + 4 * 4 + 24) * Ui::DesignSystem::scaleFactor();
    const QRectF otherColorsLabelRect(Ui::DesignSystem::layout().px12(), otherColorsLabelTop,
                                      width() - Ui::DesignSystem::layout().px24(),
                                      Ui::DesignSystem::layout().px16());
    painter.drawText(otherColorsLabelRect, Qt::AlignLeft | Qt::AlignVCenter, tr("User colors"));
    //
    // ... кнопка добавления
    //
    const auto penWidth = painter.pen().widthF() / 2;
    painter.drawEllipse(d->addCustomColorRect.adjusted(penWidth, penWidth, -penWidth, -penWidth));
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.drawText(d->addCustomColorRect, Qt::AlignCenter, u8"\U000F0415");
}

void ColorPallete::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

    update();
}

void ColorPallete::mousePressEvent(QMouseEvent* _event)
{
    if (d->addCustomColorRect.contains(_event->pos())) {
        emit addCustomColorPressed();
        return;
    }

    for (const auto& color : std::as_const(d->colorsPalette)) {
        if (!color.rect.contains(_event->pos())) {
            continue;
        }

        if (d->isColorCanBeDeslected && d->selectedColor == color) {
            d->selectedColor = {};
        } else {
            d->selectedColor = color;
        }
        emit selectedColorChanged(d->selectedColor.color);

        update();

        break;
    }
}

void ColorPallete::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    //
    // При смене дизайн системы перестраиваем палитру, чтобы пересчитались кэши положений цветов
    //
    d->buildPalette();
}
