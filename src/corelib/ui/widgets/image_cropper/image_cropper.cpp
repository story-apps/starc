#include "image_cropper.h"

#include <ui/design_system/design_system.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>


namespace {
/**
 * @brief Позиция курсора относительно рамки обрезки
 */
enum class CursorPosition {
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
    CursorPosition cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition);

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
                                   const CursorPosition _cursorPosition,
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
                                                        const CursorPosition _cursorPosition,
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
                                                       const CursorPosition _cursorPosition,
                                                       const QPointF& _mouseDelta,
                                                       const QSizeF& _deltas);


    QPixmap sourceImage;
    QRectF croppingRect;
    QRectF lastStaticCroppingRect;
    CursorPosition currentCursorPosition = CursorPosition::Undefined;
    bool isMousePressed = false;
    bool isProportionFixed = false;
    QPointF startMousePos;
    QSizeF proportion = { 1.0, 1.0 };
    // Приращения
    // width  - приращение по x
    // height - приращение по y
    QSizeF deltas = { 1.0, 1.0 };
};

CursorPosition ImageCropper::Implementation::cursorPosition(const QRectF& _cropRect,
                                                            const QPointF& _mousePosition)
{
    const qreal indent = 10 * Ui::DesignSystem::scaleFactor();
    if (!_cropRect.adjusted(-indent, -indent, indent, indent).contains(_mousePosition)) {
        return CursorPosition::Undefined;
    }

    // Находится ли точка рядом с координатой стороны
    auto isPointNearSide = [indent](qreal _sideCoordinate, qreal _pointCoordinate) {
        return (_sideCoordinate - indent) < _pointCoordinate
            && _pointCoordinate < (_sideCoordinate + indent);
    };

    // Двухстороннее направление
    if (isPointNearSide(_cropRect.top(), _mousePosition.y())
        && isPointNearSide(_cropRect.left(), _mousePosition.x())) {
        return CursorPosition::TopLeft;
    } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())
               && isPointNearSide(_cropRect.left(), _mousePosition.x())) {
        return CursorPosition::BottomLeft;
    } else if (isPointNearSide(_cropRect.top(), _mousePosition.y())
               && isPointNearSide(_cropRect.right(), _mousePosition.x())) {
        return CursorPosition::TopRight;
    } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())
               && isPointNearSide(_cropRect.right(), _mousePosition.x())) {
        return CursorPosition::BottomRight;
    }
    // Одностороннее направление
    else if (isPointNearSide(_cropRect.left(), _mousePosition.x())) {
        return CursorPosition::Left;
    } else if (isPointNearSide(_cropRect.right(), _mousePosition.x())) {
        return CursorPosition::Right;
    } else if (isPointNearSide(_cropRect.top(), _mousePosition.y())) {
        return CursorPosition::Top;
    } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())) {
        return CursorPosition::Bottom;
    }
    // Без направления
    else {
        return CursorPosition::Middle;
    }
}

void ImageCropper::Implementation::updateCursorIcon(const QPointF& _mousePosition,
                                                    QWidget* _forWidget)
{
    QCursor cursorIcon;
    //
    switch (cursorPosition(croppingRect, _mousePosition)) {
    case CursorPosition::TopRight:
    case CursorPosition::BottomLeft:
        cursorIcon = QCursor(Qt::SizeBDiagCursor);
        break;
    case CursorPosition::TopLeft:
    case CursorPosition::BottomRight:
        cursorIcon = QCursor(Qt::SizeFDiagCursor);
        break;
    case CursorPosition::Top:
    case CursorPosition::Bottom:
        cursorIcon = QCursor(Qt::SizeVerCursor);
        break;
    case CursorPosition::Left:
    case CursorPosition::Right:
        cursorIcon = QCursor(Qt::SizeHorCursor);
        break;
    case CursorPosition::Middle:
        cursorIcon = isMousePressed ? QCursor(Qt::ClosedHandCursor) : QCursor(Qt::OpenHandCursor);
        break;
    case CursorPosition::Undefined:
    default:
        cursorIcon = QCursor(Qt::ArrowCursor);
        break;
    }

    _forWidget->setCursor(cursorIcon);
}

const QRectF ImageCropper::Implementation::calculateGeometry(const QRectF& _sourceGeometry,
                                                             const CursorPosition _cursorPosition,
                                                             const QPointF& _mouseDelta)
{
    QRectF resultGeometry;
    //
    if (isProportionFixed) {
        resultGeometry = calculateGeometryWithFixedProportions(_sourceGeometry, _cursorPosition,
                                                               _mouseDelta, deltas);
    } else {
        resultGeometry
            = calculateGeometryWithCustomProportions(_sourceGeometry, _cursorPosition, _mouseDelta);
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
    const QRectF& _sourceGeometry, const CursorPosition _cursorPosition, const QPointF& _mouseDelta)
{
    QRectF resultGeometry = _sourceGeometry;
    //
    switch (_cursorPosition) {
    case CursorPosition::TopLeft:
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        break;
    case CursorPosition::TopRight:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    case CursorPosition::BottomLeft:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        break;
    case CursorPosition::BottomRight:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    case CursorPosition::Top:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        break;
    case CursorPosition::Bottom:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        break;
    case CursorPosition::Left:
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        break;
    case CursorPosition::Right:
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    default:
        break;
    }
    //
    return resultGeometry;
}

const QRectF ImageCropper::Implementation::calculateGeometryWithFixedProportions(
    const QRectF& _sourceGeometry, const CursorPosition _cursorPosition, const QPointF& _mouseDelta,
    const QSizeF& _deltas)
{
    QRectF resultGeometry = _sourceGeometry;
    //
    switch (_cursorPosition) {
    case CursorPosition::Left:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.x() * _deltas.height());
        resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        break;
    case CursorPosition::Right:
        resultGeometry.setTop(_sourceGeometry.top() - _mouseDelta.x() * _deltas.height());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        break;
    case CursorPosition::Top:
        resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() - _mouseDelta.y() * _deltas.width());
        break;
    case CursorPosition::Bottom:
        resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
        resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.y() * _deltas.width());
        break;
    case CursorPosition::TopLeft:
        if ((_mouseDelta.x() * _deltas.height()) < (_mouseDelta.y())) {
            resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.x() * _deltas.height());
            resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        } else {
            resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
            resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.y() * _deltas.width());
        }
        break;
    case CursorPosition::TopRight:
        if ((_mouseDelta.x() * _deltas.height() * -1) < (_mouseDelta.y())) {
            resultGeometry.setTop(_sourceGeometry.top() - _mouseDelta.x() * _deltas.height());
            resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
        } else {
            resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
            resultGeometry.setRight(_sourceGeometry.right() - _mouseDelta.y() * _deltas.width());
        }
        break;
    case CursorPosition::BottomLeft:
        if ((_mouseDelta.x() * _deltas.height()) < (_mouseDelta.y() * -1)) {
            resultGeometry.setBottom(_sourceGeometry.bottom() - _mouseDelta.x() * _deltas.height());
            resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
        } else {
            resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
            resultGeometry.setLeft(_sourceGeometry.left() - _mouseDelta.y() * _deltas.width());
        }
        break;
    case CursorPosition::BottomRight:
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


// ****


ImageCropper::ImageCropper(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setMouseTracking(true);
}

ImageCropper::~ImageCropper() = default;

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
    painter.drawRect(d->croppingRect);
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
    const qreal topYCoord = d->croppingRect.top() - px2;
    const qreal middleYCoord = d->croppingRect.center().y() - px2;
    const qreal bottomYCoord = d->croppingRect.bottom() - px2;
    //
    const qreal px5 = 5 * Ui::DesignSystem::scaleFactor();
    const QSizeF pointSize(px5, px5);
    //
    QVector<QRectF> points;
    points
        // левая сторона
        << QRectF(QPointF(leftXCoord, topYCoord), pointSize)
        << QRectF(QPointF(leftXCoord, middleYCoord), pointSize)
        << QRectF(QPointF(leftXCoord, bottomYCoord), pointSize)
        // центр
        << QRectF(QPointF(centerXCoord, topYCoord), pointSize)
        << QRectF(QPointF(centerXCoord, middleYCoord), pointSize)
        << QRectF(QPointF(centerXCoord, bottomYCoord), pointSize)
        // правая сторона
        << QRectF(QPointF(rightXCoord, topYCoord), pointSize)
        << QRectF(QPointF(rightXCoord, middleYCoord), pointSize)
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
        painter.drawLine(QPointF(d->croppingRect.center().x(), d->croppingRect.top()),
                         QPointF(d->croppingRect.center().x(), d->croppingRect.bottom()));
        // ... горизонтальная
        painter.drawLine(QPointF(d->croppingRect.left(), d->croppingRect.center().y()),
                         QPointF(d->croppingRect.right(), d->croppingRect.center().y()));
    }
}

void ImageCropper::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton) {
        d->isMousePressed = true;
        d->startMousePos = _event->pos();
        d->lastStaticCroppingRect = d->croppingRect;
    }
    //
    d->updateCursorIcon(_event->pos(), this);
}

void ImageCropper::mouseMoveEvent(QMouseEvent* _event)
{
    const int cropperBoundaryShift = 1 * Ui::DesignSystem::scaleFactor();
    //
    // Позиция относительно себя
    //
    const QPointF mousePos(_event->pos());
    if (!d->isMousePressed) {
        // Обработка обычного состояния, т.е. не изменяется размер
        // области обрезки, и она не перемещается по виджету
        d->currentCursorPosition = d->cursorPosition(d->croppingRect, mousePos);
        d->updateCursorIcon(mousePos, this);
        return;
    }

    if (d->currentCursorPosition == CursorPosition::Undefined) {
        return;
    }

    //
    // Определим позицию курсора внутри виджета
    //
    QPointF correctedMousePos;
    if (mousePos.x() < d->startMousePos.x()) {
        correctedMousePos.setX(std::clamp(mousePos.x(), 0.0, d->startMousePos.x()));
    } else {
        correctedMousePos.setX(
            std::clamp(mousePos.x(), d->startMousePos.x(), static_cast<qreal>(width())));
    }
    //
    if (mousePos.y() < d->startMousePos.y()) {
        correctedMousePos.setY(std::clamp(mousePos.y(), 0.0, d->startMousePos.y()));
    } else {
        correctedMousePos.setY(
            std::clamp(mousePos.y(), d->startMousePos.y(), static_cast<qreal>(height())));
    }

    //
    // Обработка действий над областью обрезки
    //
    // ... определим смещение курсора мышки
    //
    const QPointF mouseDelta(correctedMousePos.x() - d->startMousePos.x(),
                             correctedMousePos.y() - d->startMousePos.y());
    //
    // ... изменяем размер области обрезки
    //
    if (d->currentCursorPosition != CursorPosition::Middle) {
        QRectF newGeometry
            = d->calculateGeometry(d->lastStaticCroppingRect, d->currentCursorPosition, mouseDelta);

        //
        // ... пользователь пытается вывернуть область обрезки наизнанку
        //
        if (newGeometry.isNull()) {
            return;
        }

        const int boundCroppingArea = 10;
        //
        // ... пользователь пытается вытянуть область обрезки за пределы виджета
        //
        if (!QRectF(rect()).contains(newGeometry)) {
            if (d->currentCursorPosition == CursorPosition::Left) {
                if (newGeometry.x() - boundCroppingArea <= 0) {
                    newGeometry.setLeft(0);
                }
            } else if (d->currentCursorPosition == CursorPosition::Right) {
                if (newGeometry.x() + newGeometry.width() + boundCroppingArea >= width()) {
                    newGeometry.setRight(width() - cropperBoundaryShift);
                }
            } else if (d->currentCursorPosition == CursorPosition::Top) {
                if (newGeometry.y() - boundCroppingArea <= 0) {
                    newGeometry.setTop(0);
                }
            } else if (d->currentCursorPosition == CursorPosition::Bottom) {
                if (newGeometry.y() + newGeometry.height() + boundCroppingArea >= height()) {
                    newGeometry.setBottom(height() - cropperBoundaryShift);
                }
            } else if (d->currentCursorPosition == CursorPosition::TopLeft) {
                if (newGeometry.y() - boundCroppingArea <= 0) {
                    newGeometry.setTop(0);
                }
                if (newGeometry.x() - boundCroppingArea <= 0) {
                    newGeometry.setLeft(0);
                }
            } else if (d->currentCursorPosition == CursorPosition::TopRight) {
                if (newGeometry.y() - boundCroppingArea <= 0) {
                    newGeometry.setTop(0);
                }
                if (newGeometry.x() + newGeometry.width() + boundCroppingArea >= width()) {
                    newGeometry.setRight(width() - cropperBoundaryShift);
                }
            } else if (d->currentCursorPosition == CursorPosition::BottomLeft) {
                if (newGeometry.y() + newGeometry.height() + boundCroppingArea >= height()) {
                    newGeometry.setBottom(height() - cropperBoundaryShift);
                }
                if (newGeometry.x() - boundCroppingArea <= 0) {
                    newGeometry.setLeft(0);
                }
            } else if (d->currentCursorPosition == CursorPosition::BottomRight) {
                if (newGeometry.y() + newGeometry.height() + boundCroppingArea >= height()) {
                    newGeometry.setBottom(height() - cropperBoundaryShift);
                }
                if (newGeometry.x() + newGeometry.width() + boundCroppingArea >= width()) {
                    newGeometry.setRight(width() - cropperBoundaryShift);
                }
            }

            d->croppingRect = newGeometry;
            update();
            return;
        }

        //
        // ... обновляем область
        //
        d->croppingRect = newGeometry;
    }
    //
    // ... перемещаем область обрезки
    //
    else {
        auto newPos = d->lastStaticCroppingRect.topLeft() + mouseDelta;
        newPos.setX(
            std::clamp(newPos.x(), 0.0, width() - d->croppingRect.width() - cropperBoundaryShift));
        newPos.setY(std::clamp(newPos.y(), 0.0,
                               height() - d->croppingRect.height() - cropperBoundaryShift));
        d->croppingRect.moveTo(newPos);
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

    const qreal frameShift = 1 * Ui::DesignSystem::scaleFactor();
    int nwidth = height() * d->proportion.width() / d->proportion.height();
    int delta = 0;
    QSizeF rectSize = d->croppingRect.size();

    if (!d->isProportionFixed) {
        d->croppingRect.setRect(0, 0, width() - frameShift, height() - frameShift);
        update();
        return;
    }

    if (nwidth > width()) {
        int nheight = width() * d->proportion.height() / d->proportion.width();
        delta = (nheight - d->croppingRect.height()) / 2;

        if ((d->croppingRect.height() == nheight
             || d->croppingRect.height() == nheight - frameShift)
            && d->croppingRect.width() == width()) {
            return;
        }

        if (d->croppingRect.y() - delta < 0) {
            rectSize.scale(width(), nheight, Qt::IgnoreAspectRatio);
            d->croppingRect.moveTopLeft(QPointF(0, 0));
        } else if (d->croppingRect.y() + nheight > height()) {
            rectSize.scale(width(), nheight, Qt::IgnoreAspectRatio);
            d->croppingRect.moveTopLeft(QPointF(0, height() - nheight - frameShift));
        } else {
            rectSize.scale(width(), nheight, Qt::IgnoreAspectRatio);
            d->croppingRect.moveTopLeft(QPointF(0, d->croppingRect.y() - delta));
        }
    } else {
        int nwidth = height() * d->proportion.width() / d->proportion.height();
        delta = (width() - nwidth) / 2;

        if ((d->croppingRect.width() == nwidth || d->croppingRect.width() == nwidth - frameShift)
            && d->croppingRect.height() == height()) {
            return;
        }

        if (d->croppingRect.x() + nwidth > width()) {
            rectSize.scale(nwidth, height(), Qt::IgnoreAspectRatio);
            d->croppingRect.moveTopLeft(QPointF(width() - nwidth - frameShift, 0));
        } else if (d->croppingRect.x() - delta < 0) {
            rectSize.scale(nwidth, height(), Qt::IgnoreAspectRatio);
            d->croppingRect.moveTopLeft(QPointF(0, 0));
        } else {
            rectSize.scale(nwidth, height(), Qt::IgnoreAspectRatio);
            d->croppingRect.moveTopLeft(QPointF(d->croppingRect.x() - delta, 0));
        }
    }

    d->croppingRect.setSize(rectSize);
    update();
}
