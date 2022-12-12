#include "tree_view.h"

#include <QDomDocument>
#include <QMouseEvent>
#include <QPainter>
#include <QVariantAnimation>


class TreeView::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();


    /**
     * @brief Нужно ли пересчитывать размер отрисовываемых элементов при изменении размера
     */
    bool adjustSizeAutomatically = false;

    /**
     * @brief Индекс элемента на который наведена мышь
     */
    QModelIndex hoverIndex;

    /**
     * @brief  Декорации кнопки при клике
     */
    QPointF decorationCenterPosition;
    QRectF decorationRect;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

TreeView::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationRadiusAnimation.setStartValue(1.0);
    decorationRadiusAnimation.setDuration(240);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(420);
}

void TreeView::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


TreeView::TreeView(QWidget* _parent)
    : QTreeView(_parent)
    , d(new Implementation)
{
    setVerticalScrollMode(TreeView::ScrollPerPixel);
    viewport()->installEventFilter(this);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] {
        update();
        viewport()->update();
    });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] {
        update();
        viewport()->update();
    });
}

void TreeView::setAutoAdjustSize(bool _auto)
{
    d->adjustSizeAutomatically = _auto;
}

QSize TreeView::viewportSizeHint() const
{
    return QTreeView::viewportSizeHint();
}

void TreeView::restoreState(const QVariant& _state)
{
    std::function<void(const QDomElement&, const QModelIndex&)> readItem;
    readItem = [this, &readItem](const QDomElement& _node, const QModelIndex& _parent) {
        const auto index = model()->index(_node.attribute("row").toInt(), 0, _parent);
        if (_node.hasAttribute("current")) {
            setCurrentIndex(index);
        }

        if (!_node.hasChildNodes()) {
            return;
        }

        expand(index);
        auto child = _node.firstChildElement();
        while (!child.isNull()) {
            readItem(child, index);
            child = child.nextSiblingElement();
        }
    };

    QDomDocument domDocument;
    domDocument.setContent(QByteArray::fromHex(_state.toByteArray()));
    auto itemsNode = domDocument.firstChildElement("items");
    auto itemNode = itemsNode.firstChildElement();
    while (!itemNode.isNull()) {
        readItem(itemNode, {});
        itemNode = itemNode.nextSiblingElement();
    }
}

QVariant TreeView::saveState() const
{
    QByteArray state;
    state += "<items>";
    std::function<void(int, const QModelIndex&)> writeRow;
    writeRow = [this, &state, &writeRow](int _row, const QModelIndex& _parent) {
        const auto index = model()->index(_row, 0, _parent);
        const auto attributes = " row=\"" + QString::number(_row) + "\""
            + (index == currentIndex() ? " current=\"true\"" : "");
        if (isExpanded(index)) {
            state += "<item" + attributes.toUtf8() + ">";
            for (int row = 0; row < model()->rowCount(index); ++row) {
                writeRow(row, index);
            }
            state += "</item>";
        } else {
            state += "<item" + attributes.toUtf8() + "/>";
        }
    };
    for (int row = 0; row < model()->rowCount(); ++row) {
        writeRow(row, {});
    }
    state += "</items>";
    return state.toHex();
}

int TreeView::sizeHintForColumn(int _column) const
{
    return QTreeView::sizeHintForColumn(_column);
}

TreeView::~TreeView() = default;

bool TreeView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == viewport() && _event->type() == QEvent::MouseButtonPress) {
        auto event = static_cast<QMouseEvent*>(_event);
        d->decorationCenterPosition = event->pos();
        d->decorationRect = visualRect(indexAt(event->pos()));
        d->decorationRect.setLeft(0.0);
        d->decorationRect.setWidth(width());
        d->decorationRadiusAnimation.setEndValue(d->decorationRect.width());
        d->animateClick();
    }

    return QTreeView::eventFilter(_watched, _event);
}

void TreeView::paintEvent(QPaintEvent* _event)
{
    QTreeView::paintEvent(_event);

    //
    // Если необходимо, рисуем декорацию
    //
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setClipRect(d->decorationRect);
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().highlightedText());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(d->decorationCenterPosition,
                            d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
        painter.setClipRect(QRect(), Qt::NoClip);
    }
}

void TreeView::resizeEvent(QResizeEvent* _event)
{
    QTreeView::resizeEvent(_event);

    if (d->adjustSizeAutomatically) {
        scheduleDelayedItemsLayout();
    }
}

void TreeView::mouseMoveEvent(QMouseEvent* _event)
{
    QTreeView::mouseMoveEvent(_event);

    const auto hoverIndex = indexAt(_event->pos());
    if (d->hoverIndex == hoverIndex) {
        return;
    }

    d->hoverIndex = hoverIndex;
    emit hoverIndexChanged(d->hoverIndex);
}
