#include "color_picker.h"

#include <ui/design_system/design_system.h>

#include <QPainter>


class ColorPicker::Implementation
{
public:
    /**
     * @brief Сформировать палитру
     */
    void buildPalette();

    struct ColorItem {
        QColor color;
        QRectF rect;
    };
    QVector<ColorItem> colorsPalette;
    ColorItem selectedColor;
    QVector<QColor> customColors = {"#dfc123"};
    QRectF addCustomColorRect;
};

void ColorPicker::Implementation::buildPalette()
{
    colorsPalette.clear();

    const int colorRectColumns = 10;
    const QSizeF colorRectSize = { Ui::DesignSystem::layout().px24(),
                                   Ui::DesignSystem::layout().px24() };
    const qreal colorRectSpace = Ui::DesignSystem::layout().px4();

    //
    // Формируем первый ряд
    //
    int topMargin = Ui::DesignSystem::layout().px12();
    int leftMargin = Ui::DesignSystem::layout().px12();
    QList<QColor> colors;
    colors << QColor("#000000")
           << QColor("#434343")
           << QColor("#666666")
           << QColor("#999999")
           << QColor("#B7B7B7")
           << QColor("#CCCCCC")
           << QColor("#D9D9D9")
           << QColor("#EFEFEF")
           << QColor("#F3F3F3")
           << QColor("#FFFFFF");
    for (int column = 0; column < colorRectColumns; ++column) {
        QRectF colorRect;
        colorRect.setLeft(leftMargin);
        colorRect.setTop(topMargin);
        colorRect.setSize(colorRectSize);

        colorsPalette.append({ colors.at(column), colorRect });

        leftMargin += colorRectSize.width() + colorRectSpace;
    }
    topMargin += colorRectSize.height() + Ui::DesignSystem::layout().px24();


    //
    // Остальные ряды
    //
    colors.clear();
    colors << QColor("#FE0000")
           << QColor("#FF7401")
           << QColor("#FFD302")
           << QColor("#A0ED00")
           << QColor("#01CC01")
           << QColor("#06E3E4")
           << QColor("#0046F4")
           << QColor("#4F18FF")
           << QColor("#9706E7")
           << QColor("#EC0085") // ****
           << QColor("#FE3235")
           << QColor("#FF9036")
           << QColor("#FFDB34")
           << QColor("#B3F134")
           << QColor("#35D533")
           << QColor("#44D0D1")
           << QColor("#4174F0")
           << QColor("#653EE0")
           << QColor("#A048CF")
           << QColor("#F22B9E") // ****
           << QColor("#FF686A")
           << QColor("#FFAC66")
           << QColor("#FCE364")
           << QColor("#C6F567")
           << QColor("#65DF66")
           << QColor("#77DDE0")
           << QColor("#7293F4")
           << QColor("#8964EF")
           << QColor("#B74EED")
           << QColor("#F35EB2") // ****
           << QColor("#FF999C")
           << QColor("#FFC79C")
           << QColor("#FFEC99")
           << QColor("#DAF798")
           << QColor("#99EB99")
           << QColor("#B2F0F1")
           << QColor("#94B3F6")
           << QColor("#B29BF5")
           << QColor("#CF92F4")
           << QColor("#FE8ACA") // ****
           << QColor("#F4CCCC")
           << QColor("#FCE5CD")
           << QColor("#FFF2CC")
           << QColor("#D9EAD3")
           << QColor("#CAFFCA")
           << QColor("#BEFEFF")
           << QColor("#B2CEFF")
           << QColor("#CFBDF8")
           << QColor("#E3B6FF")
           << QColor("#FFAADA");
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

    topMargin += -colorRectSpace
                 + Ui::DesignSystem::layout().px12()
                 + Ui::DesignSystem::layout().px16()
                 + Ui::DesignSystem::layout().px12();
    leftMargin = Ui::DesignSystem::layout().px12();
    for (const auto& color : customColors) {
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


// ****


ColorPicker::ColorPicker(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setMouseTracking(true);

    designSystemChangeEvent(nullptr);
}

ColorPicker::~ColorPicker() = default;

QColor ColorPicker::selectedColor() const
{
    return d->selectedColor.color;
}

void ColorPicker::setSelectedColor(const QColor& _color)
{
    if (d->selectedColor.color == _color) {
        return;
    }

    for (const auto& color : d->colorsPalette) {
        if (color.color != _color) {
            continue;
        }

        d->selectedColor = color;
        break;
    }

    update();
}

void ColorPicker::paintEvent(QPaintEvent* _event)
{
    Widget::paintEvent(_event);

    Q_UNUSED(_event);

    QPainter painter( this );
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем кружки с цветами
    //
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    for (const auto& color : std::as_const(d->colorsPalette)) {
        //
        // Сам цвет
        //
        painter.setPen(Qt::NoPen);
        painter.setBrush(color.color);
        painter.drawEllipse(color.rect);

        //
        // Под мышкой
        //
        if (color.rect.contains(mousePos)) {
            const auto adjustSize = Ui::DesignSystem::layout().px2();
            const auto borderRect = color.rect.adjusted(-adjustSize, -adjustSize, adjustSize, adjustSize);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(color.color, 1.0 * Ui::DesignSystem::scaleFactor()));
            painter.drawEllipse(borderRect);
        }
    }

    //
    // Разделитель между верхней и центральной
    //
    const QRectF dividerRect(0, Ui::DesignSystem::layout().px48(), width(), Ui::DesignSystem::scaleFactor());
    painter.fillRect(dividerRect, textColor());

    //
    // Кастомные цвета
    //
    // ... заголовок
    //
    painter.setPen(QPen(textColor(), Ui::DesignSystem::layout().px2()));
    painter.setBrush(Qt::NoBrush);
    painter.setFont(Ui::DesignSystem::font().button());
    const qreal otherColorsLabelTop = (12 + 24 + 12
                                       + 12 + 24*5 + 4*4 + 12) * Ui::DesignSystem::scaleFactor();
    const QRectF otherColorsLabelRect(Ui::DesignSystem::layout().px12(), otherColorsLabelTop,
                                      width() - Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16());
    painter.drawText(otherColorsLabelRect, Qt::AlignLeft | Qt::AlignVCenter, tr("User colors"));
    //
    // ... кнопка добавления
    //
    const auto penWidth = painter.pen().widthF() / 2;
    painter.drawEllipse(d->addCustomColorRect.adjusted(penWidth, penWidth, -penWidth, -penWidth));
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.drawText(d->addCustomColorRect, Qt::AlignCenter, u8"\U000F0415");
}

void ColorPicker::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

    update();
}

void ColorPicker::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->buildPalette();
}
