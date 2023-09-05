#include "images_list_preview.h"

#include <domain/document_object.h>
#include <ui/design_system/design_system.h>

#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>


namespace Ui {

namespace {
constexpr int kInvalidImageIndex = -1;

qreal margin()
{
    return DesignSystem::layout().px62();
}

enum class CursorAction {
    Close,
    ToPrevious,
    ToNext,
};

} // namespace


class ImagesListPreview::Implementation
{
public:
    explicit Implementation(ImagesListPreview* _q);

    /**
     * @brief Обновить размер текущего изображения в соответствии с заданным размером
     */
    void updateImageGeometry();

    /**
     * @brief Текущее действие курсора
     */
    CursorAction cursorAction() const;


    ImagesListPreview* q = nullptr;

    /**
     * @brief Список изображений для отображения
     */
    QVector<Domain::DocumentImage> images;

    /**
     * @brief Текущее открытое изображение
     */
    int currentImageIndex = kInvalidImageIndex;
    QPixmap currentImage;
    QVariantAnimation opacityAnimation;
    QVariantAnimation geometryAnimation;
};

ImagesListPreview::Implementation::Implementation(ImagesListPreview* _q)
    : q(_q)
{
    opacityAnimation.setDuration(120);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    geometryAnimation.setDuration(160);
    geometryAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void ImagesListPreview::Implementation::updateImageGeometry()
{
    if (images.isEmpty() || currentImageIndex == kInvalidImageIndex) {
        return;
    }

    const QSizeF maximumSize(q->width() - margin() * 2, q->height() - margin() * 2);
    QSizeF targetSize = images.at(currentImageIndex).image.size();
    if (targetSize.width() > maximumSize.width() || targetSize.height() > maximumSize.height()) {
        targetSize = targetSize.scaled(maximumSize, Qt::KeepAspectRatio);
    }
    const QRectF contentRect(
        QPointF((q->width() - targetSize.width()) / 2.0, (q->height() - targetSize.height()) / 2.0),
        targetSize);
    geometryAnimation.setEndValue(contentRect);
}

CursorAction ImagesListPreview::Implementation::cursorAction() const
{
    const auto cursorPos = q->mapFromGlobal(QCursor::pos());
    if (const QRectF goToPrevRect(0, margin(), margin(), q->height() - margin() * 2);
        goToPrevRect.contains(cursorPos) && currentImageIndex > 0 && images.size() > 1) {
        return CursorAction::ToPrevious;
    } else if (const QRectF goToNextRect(q->width() - margin(), margin(), margin(),
                                         q->height() - margin() * 2);
               goToNextRect.contains(cursorPos) && currentImageIndex < images.size() - 1
               && images.size() > 1) {
        return CursorAction::ToNext;
    } else {
        return CursorAction::Close;
    }
}


// ****


ImagesListPreview::ImagesListPreview(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    Q_ASSERT(_parent);

    _parent->installEventFilter(this);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    resize(_parent->size());

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImagesListPreview::update));
    connect(&d->geometryAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImagesListPreview::update));
    connect(&d->geometryAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->geometryAnimation.direction() == QVariantAnimation::Backward) {
            d->currentImageIndex = kInvalidImageIndex;
            hide();
        }
    });

    //
    // По-умолчанию скрываем виджет
    //
    hide();
}

ImagesListPreview::~ImagesListPreview() = default;

void ImagesListPreview::setImages(const QVector<Domain::DocumentImage>& _images)
{
    d->currentImageIndex = 0;
    d->images = _images;
}

void ImagesListPreview::showPreview(int _imageIndex, const QRectF& _sourceRect)
{
    if (parentWidget() == nullptr) {
        return;
    }

    if (isVisible() && d->currentImageIndex == _imageIndex) {
        return;
    }

    if (_imageIndex < 0 || _imageIndex >= d->images.size()) {
        return;
    }

    if (d->images.at(_imageIndex).image.isNull()) {
        return;
    }

    d->currentImageIndex = _imageIndex;
    d->currentImage = d->images.at(_imageIndex).image;

    //
    // Настроим анимацию геометрии
    //
    d->updateImageGeometry();

    //
    // Показать изображение, подняв его с виджета
    //
    if (!isVisible()) {
        raise();
        move(0, 0);
        show();
        setFocus();

        //
        // Прозрачность
        //
        d->opacityAnimation.setDirection(QAbstractAnimation::Forward);
        d->opacityAnimation.start();

        //
        // Геометрия
        //
        d->geometryAnimation.setStartValue(_sourceRect);
        d->geometryAnimation.setDirection(QAbstractAnimation::Forward);
        d->geometryAnimation.start();
    }
    //
    // Перейти к другому изображения в уже открытом предпросмотре
    //
    else {
        emit currentItemIndexChanged(d->currentImageIndex);
        update();
    }
}

void ImagesListPreview::setCurrentImageSourceRect(const QRectF& _sourceRect)
{
    d->geometryAnimation.setStartValue(_sourceRect);
}

void ImagesListPreview::hidePreview()
{
    if (!isVisible()) {
        return;
    }

    d->opacityAnimation.setDirection(QAbstractAnimation::Backward);
    d->opacityAnimation.start();
    d->geometryAnimation.setDirection(QAbstractAnimation::Backward);
    d->geometryAnimation.start();
}

bool ImagesListPreview::event(QEvent* _event)
{
    switch (_event->type()) {
    case QEvent::ParentAboutToChange: {
        parentWidget()->removeEventFilter(this);
        break;
    }

    case QEvent::ParentChange: {
        parentWidget()->installEventFilter(this);
        resize(parentWidget()->size());
        break;
    }

    default: {
        break;
    }
    }

    return Widget::event(_event);
}

bool ImagesListPreview::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parentWidget() && _event->type() == QEvent::Resize) {
        resize(parentWidget()->size());
        d->updateImageGeometry();
    }

    return Widget::eventFilter(_watched, _event);
}

void ImagesListPreview::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setOpacity(d->opacityAnimation.currentValue().toReal());
    painter.fillRect(_event->rect(), Ui::DesignSystem::color().shadow());

    painter.setPen(Ui::DesignSystem::color().onShadow());
    painter.setFont(Ui::DesignSystem::font().iconsBig());
    const auto cursorAction = d->cursorAction();
    if (d->currentImageIndex > 0 && d->images.size() > 1) {
        const QRectF goToPrevRect(0, margin(), margin(), height() - margin() * 2);
        if (cursorAction == CursorAction::ToPrevious) {
            painter.fillRect(goToPrevRect, Ui::DesignSystem::color().shadow());
        }
        painter.drawText(goToPrevRect, Qt::AlignCenter, u8"\U000F0141");
    }
    if (d->currentImageIndex < d->images.size() - 1 && d->images.size() > 1) {
        const QRectF goToNextRect(width() - margin(), margin(), margin(), height() - margin() * 2);
        if (cursorAction == CursorAction::ToNext) {
            painter.fillRect(goToNextRect, Ui::DesignSystem::color().shadow());
        }
        painter.drawText(goToNextRect, Qt::AlignCenter, u8"\U000F0142");
    }

    painter.setOpacity(1.0);
    const Qt::TransformationMode mode = d->geometryAnimation.state() == QAbstractAnimation::Running
        ? Qt::FastTransformation
        : Qt::SmoothTransformation;
    const auto imageRect = d->geometryAnimation.currentValue().toRectF();
    painter.drawPixmap(
        imageRect.topLeft(),
        d->currentImage.scaled(imageRect.size().toSize(), Qt::KeepAspectRatio, mode));
}

void ImagesListPreview::keyPressEvent(QKeyEvent* _event)
{
    Widget::keyPressEvent(_event);

    switch (_event->key()) {
    case Qt::Key_Escape: {
        hidePreview();
        _event->accept();
        break;
    }

    case Qt::Key_Left:
    case Qt::Key_Backspace: {
        showPreview(d->currentImageIndex - 1);
        _event->accept();
        break;
    }

    case Qt::Key_Right:
    case Qt::Key_Space: {
        showPreview(d->currentImageIndex + 1);
        _event->accept();
        break;
    }

    default: {
        break;
    }
    }
}

void ImagesListPreview::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

    update();
}

void ImagesListPreview::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    if (!rect().contains(_event->pos())) {
        return;
    }

    switch (d->cursorAction()) {
    case CursorAction::ToPrevious: {
        showPreview(d->currentImageIndex - 1);
        break;
    }

    case CursorAction::ToNext: {
        showPreview(d->currentImageIndex + 1);
        break;
    }

    case CursorAction::Close: {
        hidePreview();
        break;
    }
    }
}

} // namespace Ui
