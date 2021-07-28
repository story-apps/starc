#include "page_layout.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QPaintEvent>
#include <QPainter>


class PageLayout::Implementation
{
public:
    QPageSize::PageSizeId pageSize = QPageSize::A4;
    QMarginsF margins = { 20.0, 20.0, 20.0, 20.0 };
    Qt::Alignment pageNumbersAlignment = Qt::AlignRight | Qt::AlignTop;
    qreal pageSplitter = 0.5;

    PageLayoutItem currentItem = PageLayoutItem::None;
};


// ****


PageLayout::PageLayout(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
}

PageLayout::~PageLayout() = default;

void PageLayout::setPageSize(QPageSize::PageSizeId _pageSize)
{
    if (d->pageSize == _pageSize) {
        return;
    }

    d->pageSize = _pageSize;
    update();
}

void PageLayout::setMargins(const QMarginsF& _margins)
{
    if (d->margins == _margins) {
        return;
    }

    d->margins = _margins;
    update();
}

void PageLayout::setPageNumberAlignment(Qt::Alignment _alignment)
{
    if (d->pageNumbersAlignment == _alignment) {
        return;
    }

    d->pageNumbersAlignment = _alignment;
    update();
}

void PageLayout::setPageSplitter(qreal _splitAt)
{
    if (qFuzzyCompare(d->pageSplitter, _splitAt)) {
        return;
    }

    d->pageSplitter = _splitAt;
    update();
}

void PageLayout::setCurrentItem(PageLayoutItem _item)
{
    if (d->currentItem == _item) {
        return;
    }

    d->currentItem = _item;
    update();
}

void PageLayout::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Фон
    //
    const auto backgroundRect = _event->rect();
    painter.fillRect(backgroundRect, backgroundColor());

    //
    // Фон страницы
    //
    const QSizeF pageSizeMm = QPageSize(d->pageSize).size(QPageSize::Millimeter);
    const QSizeF pageSize
        = pageSizeMm.scaled(contentsRect().width(), contentsRect().height(), Qt::KeepAspectRatio);
    const QRectF pageRect({ (contentsRect().width() - pageSize.width()) / 2.0,
                            (contentsRect().height() - pageSize.height()) / 2.0 },
                          pageSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(ColorHelper::nearby(backgroundColor()));
    painter.drawRoundedRect(pageRect, Ui::DesignSystem::card().borderRadius(),
                            Ui::DesignSystem::card().borderRadius());

    //
    // Метод для отрисовки текущего выделенного элемента особым стилем
    //
    auto drawItem = [this, &painter](PageLayoutItem _item, const std::function<void()>& _draw) {
        if (d->currentItem == _item) {
            painter.save();
            auto pen = painter.pen();
            pen.setBrush(Ui::DesignSystem::color().secondary());
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
        }
        _draw();
        if (d->currentItem == _item) {
            painter.restore();
        }
    };

    //
    // Поля
    //
    const auto scaleCoefficient = pageSize.width() / pageSizeMm.width();
    const auto marginsRect = pageRect.marginsRemoved(d->margins * scaleCoefficient);
    painter.setPen(
        QPen(ColorHelper::transparent(textColor(), Ui::DesignSystem::focusBackgroundOpacity()),
             Ui::DesignSystem::layout().px2(), Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    drawItem(PageLayoutItem::LeftMargin, [&painter, marginsRect] {
        painter.drawLine(marginsRect.topLeft(), marginsRect.bottomLeft());
    });
    drawItem(PageLayoutItem::TopMargin, [&painter, marginsRect] {
        painter.drawLine(marginsRect.topLeft(), marginsRect.topRight());
    });
    drawItem(PageLayoutItem::RightMargin, [&painter, marginsRect] {
        painter.drawLine(marginsRect.topRight(), marginsRect.bottomRight());
    });
    drawItem(PageLayoutItem::BottomMargin, [&painter, marginsRect] {
        painter.drawLine(marginsRect.bottomRight(), marginsRect.bottomLeft());
    });

    //
    // Разделитель
    //
    drawItem(PageLayoutItem::Splitter, [this, &painter, marginsRect] {
        const auto x = marginsRect.left() + marginsRect.width() * d->pageSplitter;
        painter.drawLine(x, marginsRect.top(), x, marginsRect.bottom());
    });

    //
    // Номера страниц
    //
    QRectF pageNumbersRect;
    switch (d->pageNumbersAlignment & Qt::AlignHorizontal_Mask) {
    case Qt::AlignLeft: {
        pageNumbersRect.setLeft(pageRect.left());
        pageNumbersRect.setRight(marginsRect.left());
        ;
        break;
    }
    case Qt::AlignHCenter: {
        pageNumbersRect.setLeft(marginsRect.left());
        pageNumbersRect.setRight(marginsRect.right());
        break;
    }
    case Qt::AlignRight: {
        pageNumbersRect.setLeft(marginsRect.right());
        pageNumbersRect.setRight(pageRect.right());
        break;
    }
    }
    //
    switch (d->pageNumbersAlignment & Qt::AlignVertical_Mask) {
    case Qt::AlignTop: {
        pageNumbersRect.setTop(pageRect.top());
        pageNumbersRect.setBottom(marginsRect.top());
        break;
    }
    case Qt::AlignBottom: {
        pageNumbersRect.setTop(marginsRect.bottom());
        pageNumbersRect.setBottom(pageRect.bottom());
        break;
    }
    }
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.setPen(textColor());
    drawItem(PageLayoutItem::PageNumber, [&painter, pageNumbersRect] {
        painter.drawText(pageNumbersRect, Qt::AlignCenter, u8"\U000F03A0");
    });
}
