#include "image_cropper.h"

#include <ui/design_system/design_system.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>


namespace {
/**
 * @brief Направление курсора относительно рамки обрезки
 */
enum class CursorDirection {
    Undefined,
    Middle,
    Top,
    Bottom,
    Left,
    Right,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};
} // namespace

class ImageCropper::Implementation
{
public:
    /**
     * @brief Определение местоположения курсора над виджетом
     */
    CursorDirection cursorDirection(const QRectF& _cropRect, const QPointF& _mousePosition);

    /**
     * @brief Обновить иконку курсора соответствующую местоположению мыши
     */
    void updateCursorIcon(const QPointF& _mousePosition, QWidget* _forWidget);

    /**
     * @brief Получить размер виджета после его изменения мышью
     * --------
     * Контракты:
     * 1. Метод должен вызываться, только при зажатой кнопке мыши
     *    (т.е. при перемещении или изменении размера виджета)
     * --------
     * В случае неудачи возвращает null-прямоугольник
     */
    const QRectF calculateGeometry(const QRectF& _sourceGeometry,
                                   const CursorDirection _cursorDirection,
                                   const QPointF& _mouseDelta);

    /**
     * @brief Получить размер виджета после его изменения мышью
     * Метод изменяет виджет не сохраняя начальных пропорций сторон
     * ------
     * Контракты:
     * 1. Метод должен вызываться, только при зажатой кнопке мыши
     *    (т.е. при перемещении или изменении размера виджета)
     */
    const QRectF calculateGeometryWithCustomProportions(const QRectF& _sourceGeometry,
                                                        const CursorDirection _cursorDirection,
                                                        const QPointF& _mouseDelta);

    /**
     * @brief Получить размер виджета после его изменения мышью
     * Метод изменяет виджет сохраняя начальные пропорции сторон
     * ------
     * Контракты:
     * 1. Метод должен вызываться, только при зажатой кнопке мыши
     *    (т.е. при перемещении или изменении размера виджета)
     */
    const QRectF calculateGeometryWithFixedProportions(const QRectF& _sourceGeometry,
                                                       const CursorDirection _cursorDirection,
                                                       const QPointF& _mouseDelta,
                                                       const QSizeF& _deltas);

    /**
     * @brief Изменить размер области выделения
     */
    void resizeCroppingRect(const QPointF& _mousePos, const QRectF& _rect);

    /**
     * @brief Передвинуть область выделения
     */
    void moveCroppingRect(const QPointF& _mousePos, const QSizeF& _size);


    /**
     * @brief Изображение для обрезки
     */
    QPixmap sourceImage;

    /**
     * @brief Область выделения
     */
    QRectF croppingRect;

    /**
     * @brief Область обрезки зафиксированная перед началом изменения области
     */
    QRectF lastCroppingRect;

    /**
     * @brief Направление курсора
     */
    CursorDirection currentCursorDirection = CursorDirection::Undefined;

    bool isMousePressed = false;
    bool isProportionFixed = false;
    QPointF startMousePos;
    QSizeF proportion = { 1.0, 1.0 };
    // Приращения
    // width  - приращение по x
    // height - приращение по y
    QSizeF deltas = { 1.0, 1.0 };
};

CursorDirection ImageCropper::Implementation::cursorDirection(const QRectF& _cropRect,
                                                              const QPointF& _mousePosition)
{
    const qreal indent = 10 * Ui::DesignSystem::scaleFactor();
    if (!_cropRect.adjusted(-indent, -indent, indent, indent).contains(_mousePosition)) {
        return CursorDirection::Undefined;
    }

    // Находится ли точка рядом с координатой стороны
    auto isPointNearSide = [indent](qreal _sideCoordinate, qreal _pointCoordinate) {
        return (_sideCoordinate - indent) < _pointCoordinate
            && _pointCoordinate < (_sideCoordinate + indent);
    };

    // Двухстороннее направление
    if (isPointNearSide(_cropRect.top(), _mousePosition.y())
        && isPointNearSide(_cropRect.left(), _mousePosition.x())) {
        return CursorDirection::TopLeft;
    } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())
               && isPointNearSide(_cropRect.left(), _mousePosition.x())) {
        return CursorDirection::BottomLeft;
    } else if (isPointNearSide(_cropRect.top(), _mousePosition.y())
               && isPointNearSide(_cropRect.right(), _mousePosition.x())) {
        return CursorDirection::TopRight;
    } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())
               && isPointNearSide(_cropRect.right(), _mousePosition.x())) {
        return CursorDirection::BottomRight;
    }
    // Одностороннее направление
    else if (isPointNearSide(_cropRect.left(), _mousePosition.x())) {
        return CursorDirection::Left;
    } else if (isPointNearSide(_cropRect.right(), _mousePosition.x())) {
        return CursorDirection::Right;
    } else if (isPointNearSide(_cropRect.top(), _mousePosition.y())) {
        return CursorDirection::Top;
    } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())) {
        return CursorDirection::Bottom;
    }
    // Без направления
    else {
        return CursorDirection::Middle;
    }
}

void ImageCropper::Implementation::updateCursorIcon(const QPointF& _mousePosition,
                                                    QWidget* _forWidget)
{
    QCursor cursorIcon;
    //
    switch (cursorDirection(croppingRect, _mousePosition)) {
    case CursorDirection::TopRight:
    case CursorDirection::BottomLeft:
        cursorIcon = QCursor(Qt::SizeBDiagCursor);
        break;
    case CursorDirection::TopLeft:
    case CursorDirection::BottomRight:
        cursorIcon = QCursor(Qt::SizeFDiagCursor);
        break;
    case CursorDirection::Top:
    case CursorDirection::Bottom:
        cursorIcon = QCursor(Qt::SizeVerCursor);
        break;
    case CursorDirection::Left:
    case CursorDirection::Right:
        cursorIcon = QCursor(Qt::SizeHorCursor);
        break;
    case CursorDirection::Middle:
        cursorIcon = isMousePressed ? QCursor(Qt::ClosedHandCursor) : QCursor(Qt::OpenHandCursor);
        break;
    case CursorDirection::Undefined:
    default:
        cursorIcon = QCursor(Qt::ArrowCursor);
        break;
    }

    _forWidget->setCursor(cursorIcon);
}

const QRectF ImageCropper::Implementation::calculateGeometry(const QRectF& _sourceGeometry,
                                                             const CursorDirection _cursorDirection,
                                                             const QPointF& _mouseDelta)
{
    QRectF resultGeometry;
    //
    if (isProportionFixed) {
        resultGeometry = calculateGeometryWithFixedProportions(_sourceGeometry, _cursorDirection,
                                                               _mouseDelta, deltas);
    } else {
        resultGeometry = calculateGeometryWithCustomProportions(_sourceGeometry, _cursorDirection,
                                                                _mouseDelta);
    }
    // Если пользователь пытается вывернуть область обрезки наизнанку,
    // возвращаем null-прямоугольник
    if (resultGeometry.left() >= resultGeometry.right()
        || resultGeometry.top() >= resultGeometry.bottom()) {
        resultGeometry = QRect();
    }
    //
    return resultGeometry;
}

const QRectF ImageCropper::Implementation::calculateGeometryWithCustomProportions(
    const QRectF& _sourceGeometry, const CursorDirection _cursorDirection,
    const QPointF& _mouseDelta)
{
    QRectF resultGeometry = _sourceGeometry;
    //
    switch (_cursorDirection) {
    case CursorDirection::TopLeft:
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        break;
    case CursorDirection::TopRight:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    case CursorDirection::BottomLeft:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        break;
    case CursorDirection::BottomRight:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    case CursorDirection::Top:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        break;
    case CursorDirection::Bottom:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        break;
    case CursorDirection::Left:
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        break;
    case CursorDirection::Right:
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    default:
        break;
    }
    //
    return resultGeometry;
}

const QRectF ImageCropper::Implementation::calculateGeometryWithFixedProportions(
    const QRectF& _sourceGeometry, const CursorDirection _cursorDirection,
    const QPointF& _mouseDelta, const QSizeF& _deltas)
{
    QRectF resultGeometry = _sourceGeometry;
    //
    switch (_cursorDirection) {
    case CursorDirection::Left:
        resultGeometry.setBottom(_sourceGeometry.bottom() - _mouseDelta.x() * _deltas.height());
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        break;
    case CursorDirection::Right:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.x() * _deltas.height());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    case CursorDirection::Top:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() - _mouseDelta.y() * _deltas.width());
        break;
    case CursorDirection::Bottom:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.y() * _deltas.width());
        break;
    case CursorDirection::TopLeft:
        if ((_mouseDelta.x() * _deltas.height()) < (_mouseDelta.y())) {
            resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.x() * _deltas.height());
            resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        } else {
            resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
            resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.y() * _deltas.width());
        }
        break;
    case CursorDirection::TopRight:
        if ((_mouseDelta.x() * _deltas.height() * -1) < (_mouseDelta.y())) {
            resultGeometry.setTop(_sourceGeometry.top() - _mouseDelta.x() * _deltas.height());
            resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        } else {
            resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
            resultGeometry.setRight(_sourceGeometry.right() - _mouseDelta.y() * _deltas.width());
        }
        break;
    case CursorDirection::BottomLeft:
        if ((_mouseDelta.x() * _deltas.height()) < (_mouseDelta.y() * -1)) {
            resultGeometry.setBottom(_sourceGeometry.bottom() - _mouseDelta.x() * _deltas.height());
            resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        } else {
            resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
            resultGeometry.setLeft(_sourceGeometry.left() - _mouseDelta.y() * _deltas.width());
        }
        break;
    case CursorDirection::BottomRight:
        if ((_mouseDelta.x() * _deltas.height()) > (_mouseDelta.y())) {
            resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.x() * _deltas.height());
            resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        } else {
            resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
            resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.y() * _deltas.width());
        }
        break;
    default:
        break;
    }
    //
    return resultGeometry;
}

void ImageCropper::Implementation::resizeCroppingRect(const QPointF& _mousePos, const QRectF& _rect)
{
    //
    // Определим позицию курсора внутри виджета в случае, если курсор вышел за его пределы
    //
    QPointF correctedMousePos;
    if (_mousePos.x() < startMousePos.x()) {
        correctedMousePos.setX(std::clamp(_mousePos.x(), 0.0, startMousePos.x()));
    } else {
        correctedMousePos.setX(
            std::clamp(_mousePos.x(), startMousePos.x(), static_cast<qreal>(_rect.width())));
    }
    //
    if (_mousePos.y() < startMousePos.y()) {
        correctedMousePos.setY(std::clamp(_mousePos.y(), 0.0, startMousePos.y()));
    } else {
        correctedMousePos.setY(
            std::clamp(_mousePos.y(), startMousePos.y(), static_cast<qreal>(_rect.height())));
    }

    //
    // Обработка действий над областью обрезки
    //
    // ... определим смещение курсора мышки
    //
    QPointF mouseDelta = correctedMousePos - startMousePos;
    //
    // ... изменяем размер области обрезки
    //
    QRectF newGeometry = calculateGeometry(lastCroppingRect, currentCursorDirection, mouseDelta);

    //
    // ... пользователь пытается вывернуть область обрезки наизнанку
    //
    if (newGeometry.isNull()) {
        return;
    }

    //
    // ... пользователь пытается вытянуть область обрезки за пределы виджета
    //
    if (!_rect.contains(newGeometry)) {
        //
        // ... ищем масимальный возможный размер для использования
        //
        QPoint mouseDeltaCorrected = mouseDelta.toPoint();
        const int xDelta = mouseDelta.x() < 0 ? 1 : -1;
        const int yDelta = mouseDelta.y() < 0 ? 1 : -1;
        while (!_rect.contains(newGeometry)) {
            if (mouseDelta.isNull()) {
                return;
            }

            if (mouseDeltaCorrected.x() != 0) {
                mouseDeltaCorrected.setX(mouseDeltaCorrected.x() + xDelta);
            }
            if (mouseDeltaCorrected.y() != 0) {
                mouseDeltaCorrected.setY(mouseDeltaCorrected.y() + yDelta);
            }
            newGeometry
                = calculateGeometry(lastCroppingRect, currentCursorDirection, mouseDeltaCorrected);
        }
    }

    //
    // ... обновляем область
    //
    croppingRect = newGeometry;
}

void ImageCropper::Implementation::moveCroppingRect(const QPointF& _mousePos, const QSizeF& _size)
{
    const auto mouseDelta = _mousePos - startMousePos;
    auto newPos = lastCroppingRect.topLeft() + mouseDelta;
    newPos.setX(std::clamp(newPos.x(), 0.0, _size.width() - croppingRect.width()));
    newPos.setY(std::clamp(newPos.y(), 0.0, _size.height() - croppingRect.height()));
    croppingRect.moveTo(newPos);
}


// ****


ImageCropper::ImageCropper(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setMouseTracking(true);
}

ImageCropper::~ImageCropper() = default;

QPixmap ImageCropper::image() const
{
    return d->sourceImage;
}

void ImageCropper::setImage(const QPixmap& _image)
{
    d->sourceImage = _image;
    update();
}

void ImageCropper::setProportion(const QSizeF& _proportion)
{
    //
    // Пропорции хранятся в коэффициентах приращения сторон
    // Таким образом, при изменении размера области выделения,
    // размеры сторон изменяются на размер зависящий от
    // коэффициентов приращения.
    //

    //
    // Сохраним пропорциональную зависимость области выделения в коэффициентах приращения сторон
    //
    if (d->proportion != _proportion) {
        d->proportion = _proportion;
        d->deltas.setHeight(_proportion.height() / _proportion.width());
        d->deltas.setWidth(_proportion.width() / _proportion.height());
    }

    //
    // Обновим пропорции области выделения
    //

    if (!d->isProportionFixed) {
        return;
    }

    const qreal croppintRectSideRelation = d->croppingRect.width() / d->croppingRect.height();
    const qreal proportionSideRelation = d->proportion.width() / d->proportion.height();
    if (qFuzzyCompare(croppintRectSideRelation, proportionSideRelation)) {
        return;
    }

    //
    // Если область выделения не соответствует необходимым пропорциям обновим её
    //
    // ... установим размер той стороны, что длиннее
    //
    if (d->croppingRect.width() < d->croppingRect.height()) {
        d->croppingRect.setHeight(d->croppingRect.width() * d->deltas.height());
    } else {
        d->croppingRect.setWidth(d->croppingRect.height() * d->deltas.width());
    }
    //
    // ... перерисуем виджет
    //
    update();
}

void ImageCropper::setProportionFixed(const bool _isFixed)
{
    if (d->isProportionFixed == _isFixed) {
        return;
    }

    d->isProportionFixed = _isFixed;
    setProportion(d->proportion);
}

const QPixmap ImageCropper::croppedImage()
{
    //
    // Получим размер отображаемого изображения
    //
    const QSizeF scaledImageSize = d->sourceImage.size().scaled(size(), Qt::KeepAspectRatio);

    //
    // Определим расстояние от левого и верхнего краёв
    //
    qreal leftDelta = 0.0;
    qreal topDelta = 0.0;
    if (size().height() == scaledImageSize.height()) {
        leftDelta = (width() - scaledImageSize.width()) / 2;
    } else {
        topDelta = (height() - scaledImageSize.height()) / 2;
    }

    //
    // Определим пропорцию области обрезки по отношению к исходному изображению
    //
    const qreal xScale = d->sourceImage.width() / scaledImageSize.width();
    const qreal yScale = d->sourceImage.height() / scaledImageSize.height();

    //
    // Расчитаем область обрезки с учётом коррекции размеров исходного изображения
    //
    QRectF realSizeRect(
        QPointF(d->croppingRect.left() - leftDelta, d->croppingRect.top() - topDelta),
        d->croppingRect.size());
    //
    // ... корректируем левый и верхний края
    //
    realSizeRect.setLeft((d->croppingRect.left() - leftDelta) * xScale);
    realSizeRect.setTop((d->croppingRect.top() - topDelta) * yScale);
    //
    // ... корректируем размер
    //
    realSizeRect.setWidth(d->croppingRect.width() * xScale);
    realSizeRect.setHeight(d->croppingRect.height() * yScale);

    //
    // Получаем обрезанное изображение
    //
    return d->sourceImage.copy(realSizeRect.toRect());
}

QSize ImageCropper::sizeHint() const
{
    if (d->sourceImage.isNull()) {
        return {};
    }

    if (maximumSize().width() < d->sourceImage.size().width()
        || maximumSize().height() < d->sourceImage.size().height()) {
        return d->sourceImage.size().scaled(maximumSize(), Qt::KeepAspectRatio);
    }

    return d->sourceImage.size();
}

void ImageCropper::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем изображение по центру виджета
    //
    // ... подгоним изображение для отображения по размеру виджета
    //
    const QPixmap scaledImage
        = d->sourceImage.scaled(size(), Qt::KeepAspectRatio, Qt::FastTransformation);
    //
    // ... рисуем изображение по центру виджета
    //
    const QPointF imagePos = height() == scaledImage.height()
        ? QPointF((width() - scaledImage.width()) / 2, 0)
        : QPointF(0, (height() - scaledImage.height()) / 2);
    painter.drawPixmap(imagePos, scaledImage);

    //
    // Рисуем область обрезки
    //

    //
    // Если это первое отображение после инициилизации, то центруем область обрезки
    //
    if (d->croppingRect.isNull()) {
        d->croppingRect.setSize(QSizeF(size()) / 2);
        if (d->isProportionFixed) {
            if (d->croppingRect.width() < d->croppingRect.height()) {
                d->croppingRect.setHeight(d->croppingRect.width() * d->deltas.height());
            } else {
                d->croppingRect.setWidth(d->croppingRect.height() * d->deltas.width());
            }
        }
        const qreal x = (width() - d->croppingRect.width()) / 2;
        const qreal y = (height() - d->croppingRect.height()) / 2;
        d->croppingRect.moveTo(x, y);
    }

    //
    // Рисуем затемненную область
    //
    QPainterPath darkRectPath;
    darkRectPath.addRect(d->croppingRect);
    darkRectPath.addRect(rect());
    painter.setBrush(Ui::DesignSystem::color().shadow());
    painter.setPen(Qt::transparent);
    painter.drawPath(darkRectPath);

    //
    // Рамка и контрольные точки
    //
    painter.setPen(textColor());
    //
    // ... рисуем прямоугольник области обрезки
    //
    painter.setBrush(Qt::transparent);
    //
    // ... немного корректируем область, чтобы линии симметрично уходили в край изображения
    //
    painter.drawRect(d->croppingRect.adjusted(-1, -1, 0, 0));
    //
    // ... рисуем контрольные точки
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(textColor());
    // Вспомогательные X координаты
    const qreal px2 = 2 * Ui::DesignSystem::scaleFactor();
    const qreal px3 = 3 * Ui::DesignSystem::scaleFactor();
    const qreal leftXCoord = d->croppingRect.left() - px3;
    const qreal centerXCoord = d->croppingRect.center().x() - px2;
    const qreal rightXCoord = d->croppingRect.right() - px2;
    // Вспомогательные Y координаты
    const qreal topYCoord = d->croppingRect.top() - px3;
    const qreal centerYCoord = d->croppingRect.center().y() - px2;
    const qreal bottomYCoord = d->croppingRect.bottom() - px2;
    //
    const qreal px5 = 5 * Ui::DesignSystem::scaleFactor();
    const QSizeF pointSize(px5, px5);
    //
    QVector<QRectF> points;
    points
        // левая сторона
        << QRectF(QPointF(leftXCoord, topYCoord), pointSize)
        << QRectF(QPointF(leftXCoord, centerYCoord), pointSize)
        << QRectF(QPointF(leftXCoord, bottomYCoord), pointSize)
        // центр
        << QRectF(QPoint(centerXCoord, topYCoord), pointSize)
        << QRectF(QPoint(centerXCoord, centerYCoord), pointSize)
        << QRectF(QPoint(centerXCoord, bottomYCoord), pointSize)
        // правая сторона
        << QRectF(QPointF(rightXCoord, topYCoord), pointSize)
        << QRectF(QPointF(rightXCoord, centerYCoord), pointSize)
        << QRectF(QPointF(rightXCoord, bottomYCoord), pointSize);
    //
    painter.drawRects(points);
    //
    // ... рисуем пунктирные линии
    //
    {
        QPen dashPen(textColor());
        dashPen.setStyle(Qt::DashLine);
        painter.setPen(dashPen);
        // ... вертикальная
        painter.drawLine(QPoint(d->croppingRect.center().x(), d->croppingRect.top()),
                         QPoint(d->croppingRect.center().x(), d->croppingRect.bottom()));
        // ... горизонтальная
        painter.drawLine(QPoint(d->croppingRect.left(), d->croppingRect.center().y()),
                         QPoint(d->croppingRect.right(), d->croppingRect.center().y()));
    }
}

void ImageCropper::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton) {
        d->isMousePressed = true;
        d->startMousePos = _event->pos();
        switch (d->currentCursorDirection) {
        case CursorDirection::Left: {
            d->startMousePos.setX(d->croppingRect.left());
            break;
        }

        case CursorDirection::TopLeft: {
            d->startMousePos.setX(d->croppingRect.left());
            d->startMousePos.setY(d->croppingRect.top());
            break;
        }

        case CursorDirection::Top: {
            d->startMousePos.setY(d->croppingRect.top());
            break;
        }

        case CursorDirection::TopRight: {
            d->startMousePos.setX(d->croppingRect.right());
            d->startMousePos.setY(d->croppingRect.top());
            break;
        }

        case CursorDirection::Right: {
            d->startMousePos.setX(d->croppingRect.right());
            break;
        }

        case CursorDirection::BottomRight: {
            d->startMousePos.setX(d->croppingRect.right());
            d->startMousePos.setY(d->croppingRect.bottom());
            break;
        }

        case CursorDirection::Bottom: {
            d->startMousePos.setY(d->croppingRect.bottom());
            break;
        }

        case CursorDirection::BottomLeft: {
            d->startMousePos.setX(d->croppingRect.left());
            d->startMousePos.setY(d->croppingRect.bottom());
            break;
        }

        default: {
            break;
        }
        }
        d->lastCroppingRect = d->croppingRect;
    }
    //
    d->updateCursorIcon(_event->pos(), this);
}

void ImageCropper::mouseMoveEvent(QMouseEvent* _event)
{
    //
    // Позиция относительно себя
    //
    const QPointF mousePos(_event->pos());
    if (!d->isMousePressed) {
        // Обработка обычного состояния, т.е. не изменяется размер
        // области обрезки, и она не перемещается по виджету
        d->currentCursorDirection = d->cursorDirection(d->croppingRect, mousePos);
        d->updateCursorIcon(mousePos, this);
        return;
    }

    switch (d->currentCursorDirection) {
    default: {
        d->resizeCroppingRect(mousePos, rect());
        break;
    }

    case CursorDirection::Middle: {
        d->moveCroppingRect(mousePos, size());
        break;
    }

    case CursorDirection::Undefined: {
        break;
    }
    }

    //
    // Перерисуем виджет
    //
    update();
}

void ImageCropper::mouseReleaseEvent(QMouseEvent* _event)
{
    d->isMousePressed = false;
    d->updateCursorIcon(_event->pos(), this);
}

void ImageCropper::mouseDoubleClickEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)

    if (d->currentCursorDirection != CursorDirection::Middle) {
        return;
    }

    //
    // Определим максимально доступный размер
    //
    QRectF newCroppingRect = d->croppingRect;
    if (d->isProportionFixed) {
        newCroppingRect.setSize(newCroppingRect.size().scaled(rect().size(), Qt::KeepAspectRatio));
        //
        // ... позиционируем по центру
        //
        newCroppingRect.moveCenter(rect().center());
        //
        // ... из-за нечётных чисел область может быть смещена на один пиксель за пределы, вернём её
        //
        if (newCroppingRect.top() < 0 || newCroppingRect.left() < 0) {
            newCroppingRect.setTopLeft(newCroppingRect.topLeft() + QPointF(1, 1));
        }
    } else {
        newCroppingRect = rect();
    }
    if (d->croppingRect == newCroppingRect) {
        return;
    }

    //
    // ... применяем с эффектом
    //
    auto animation = new QVariantAnimation(this);
    animation->setDuration(240);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(d->croppingRect);
    animation->setEndValue(newCroppingRect);
    connect(animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& _value) {
        d->croppingRect = _value.toRectF();
        update();
    });
    animation->start(QVariantAnimation::DeleteWhenStopped);
}
