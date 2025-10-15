#include "image_helper.h"

#include "color_helper.h"

#include <QBuffer>
#include <QByteArray>
#include <QCache>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

namespace {
/**
 * @brief Максимальные размеры изображения
 */
/** @{ */
const int kImageMaxWidth = 2688;
const int kImageMaxHeight = 2688;
/** @} */

/**
 * @brief Используем низкое качество изображения (всё-таки у нас приложение не для фотографов)
 */
const int kImageFileQuality = 80;
} // namespace

QByteArray ImageHelper::bytesFromImage(const QPixmap& _image)
{
    if (_image.isNull()) {
        return {};
    }

    //
    // Если необходимо корректируем размер изображения
    //
    QPixmap imageScaled = _image;
    if (imageScaled.width() > kImageMaxWidth || imageScaled.height() > kImageMaxHeight) {
        imageScaled = imageScaled.scaled(kImageMaxWidth, kImageMaxHeight, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    }

    //
    // Сохраняем изображение
    //
    QByteArray imageData;
    QBuffer imageDataBuffer(&imageData);
    imageDataBuffer.open(QIODevice::WriteOnly);
    const char* imageFormat = imageScaled.hasAlpha() ? "PNG" : "JPG";
    imageScaled.save(&imageDataBuffer, imageFormat, kImageFileQuality);
    return imageData;
}

QPixmap ImageHelper::imageFromBytes(const QByteArray& _bytes)
{
    QPixmap image;
    image.loadFromData(_bytes);
    return image;
}

QPixmap ImageHelper::loadSvg(const QString& _svgPath, const QSize& _size)
{
    //
    // Кэш изображений
    //
    using CacheKey = QPair<QString, QPair<int, int>>;
    static QCache<CacheKey, QPixmap> s_imagesCache;

    //
    // Ищем изображение в кэше
    //
    const CacheKey imageKey{ _svgPath, { _size.width(), _size.height() } };
    if (s_imagesCache.contains(imageKey)) {
        return *s_imagesCache[imageKey];
    }

    //
    // Если нет, сохраняем в кэш для дальнейшего использования
    //
    QPixmap* image = new QPixmap(QIcon(_svgPath).pixmap(_size));
    s_imagesCache.insert(imageKey, image);
    return *image;
}

QPixmap ImageHelper::loadSvg(const QString& _svgPath, const QSize& _size, const QColor& _color)
{
    //
    // Кэш изображений
    //
    using CacheKey = QPair<QString, QPair<int, QPair<int, QRgb>>>;
    static QCache<CacheKey, QPixmap> s_imagesCache;

    //
    // Ищем изображение в кэше
    //
    const CacheKey imageKey{ _svgPath, { _size.width(), { _size.height(), _color.rgb() } } };
    if (s_imagesCache.contains(imageKey)) {
        return *s_imagesCache[imageKey];
    }

    //
    // Формируем изображение
    //
    QPixmap svg = loadSvg(_svgPath, _size);
    setPixmapColor(svg, _color);

    //
    // И сохраняем в кэш для дальнейшего использования
    //
    QPixmap* image = new QPixmap(svg);
    s_imagesCache.insert(imageKey, image);
    return *image;
}

void ImageHelper::setPixmapColor(QPixmap& _pixmap, const QColor& _color)
{
    if (_pixmap.isNull()) {
        return;
    }

    QPixmap colorizedPixmap = _pixmap;
    QPainter painter(&colorizedPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(colorizedPixmap.rect(), _color);
    painter.end();
    _pixmap.swap(colorizedPixmap);
}

bool ImageHelper::isImagesEqual(const QPixmap& _lhs, const QPixmap& _rhs)
{
    return bytesFromImage(_lhs) == bytesFromImage(_rhs);
}

QPixmap ImageHelper::makeAvatar(const QString& _text, const QFont& _font, const QSize& _size,
                                const QColor& _textColor)
{
    //
    // Кэш аватарок
    //
    using CacheKey = QPair<QString, QPair<int, QPair<int, QRgb>>>;
    static QCache<CacheKey, QPixmap> s_avatarsCache;

    //
    // Ищем аватарку в кэше
    //
    const CacheKey avatarKey{ _text, { _size.width(), { _font.pixelSize(), _textColor.rgb() } } };
    if (s_avatarsCache.contains(avatarKey)) {
        return *s_avatarsCache[avatarKey];
    }

    //
    // Если нет в кэше, то рисуем
    //
    QImage avatar(_size, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Формируем цвет для подложки заданного текста
    //
    const QColor avatarColor = ColorHelper::forText(_text);

    //
    // Рисуем фон авки
    //
    painter.setPen(QPen(avatarColor, 2.0));
    painter.setBrush(avatarColor);
    const QRectF avatarRect = QRectF(QPointF(0.0, 0.0), _size).adjusted(2, 2, -2, -2);
    painter.drawEllipse(avatarRect);

    //
    // Сформируем текст для отображения
    //
    QString textToDraw = _text.simplified().toUpper();
    if (textToDraw.count(' ') > 0) {
        const QStringList words = textToDraw.split(' ', Qt::SkipEmptyParts);
        textToDraw = QString("%1%2").arg(words[0][0]).arg(words[1][0]);
    } else {
        textToDraw = textToDraw.left(2);
    }
    //
    // Рисуем текст
    //
    painter.setPen(_textColor);
    painter.setFont(_font);
    painter.drawText(avatarRect, Qt::AlignCenter, textToDraw);
    painter.end();

    //
    // И сохраняем в кэш для дальнейшего использования
    //
    QPixmap* avatarPixmap = new QPixmap(QPixmap::fromImage(avatar));
    s_avatarsCache.insert(avatarKey, avatarPixmap);
    return *avatarPixmap;
}

QPixmap ImageHelper::makeAvatar(const QPixmap& _pixmap, const QSize& _size)
{
    return makeAvatar(_pixmap, _size, std::max(_size.width(), _size.height()) / 2);
}

QPixmap ImageHelper::makeAvatar(const QPixmap& _pixmap, const QSize& _size, int _radius)
{
    //
    // Кэш аватарок
    //
    using CacheKey = QPair<qint64, QPair<int, int>>;
    static QCache<CacheKey, QPixmap> s_avatarsCache;

    //
    // Ищем аватарку в кэше
    //
    const CacheKey avatarKey{ _pixmap.cacheKey(), { _size.width(), _radius } };
    if (s_avatarsCache.contains(avatarKey)) {
        return *s_avatarsCache[avatarKey];
    }

    //
    // Если нет в кэше, то рисуем
    //
    QImage avatar = QImage(_size, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath circleClipPath;
    if (_radius == std::max(_size.width(), _size.height()) / 2) {
        circleClipPath.addEllipse(avatar.rect().adjusted(1, 1, 0, 0).center(), avatar.width() / 2,
                                  avatar.height() / 2);
    } else {
        circleClipPath.addRoundedRect(avatar.rect(), _radius, _radius);
    }
    painter.setClipPath(circleClipPath);
    painter.drawPixmap(
        0, 0, _pixmap.scaled(_size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    painter.end();

    //
    // И сохраняем в кэш для дальнейшего использования
    //
    QPixmap* avatarPixmap = new QPixmap(QPixmap::fromImage(avatar));
    s_avatarsCache.insert(avatarKey, avatarPixmap);
    return *avatarPixmap;
}

QPixmap ImageHelper::makeAvatar(const QPixmap& _pixmap, const QSize& _size,
                                const QColor& _backgroundColor)
{
    //
    // Кэш аватарок
    //
    using CacheKey = QPair<qint64, QPair<int, QRgb>>;
    static QCache<CacheKey, QPixmap> s_avatarsCache;

    //
    // Ищем аватарку в кэше
    //
    const CacheKey avatarKey{ _pixmap.cacheKey(), { _size.width(), _backgroundColor.rgba() } };
    if (s_avatarsCache.contains(avatarKey)) {
        return *s_avatarsCache[avatarKey];
    }

    //
    // Если нет в кэше, то рисуем
    //
    QImage avatar(_size, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем фон авки
    //
    painter.setPen(QPen(_backgroundColor, 2.0));
    painter.setBrush(_backgroundColor);
    const QRectF avatarRect = QRectF(QPointF(0.0, 0.0), _size).adjusted(2, 2, -2, -2);
    painter.drawEllipse(avatarRect);

    //
    // Рисуем в центре заданное изображение
    //
    const QRectF pixmapRect(
        QPointF(_size.width() - _pixmap.width(), _size.height() - _pixmap.height()) / 2.0,
        _pixmap.size());
    painter.drawPixmap(pixmapRect.toRect(), _pixmap);
    painter.end();

    //
    // И сохраняем в кэш для дальнейшего использования
    //
    QPixmap* avatarPixmap = new QPixmap(QPixmap::fromImage(avatar));
    s_avatarsCache.insert(avatarKey, avatarPixmap);
    return *avatarPixmap;
}

QT_BEGIN_NAMESPACE
extern void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality, bool alphaOnly,
                         int transposed = 0);
QT_END_NAMESPACE

QPixmap ImageHelper::blurImage(const QPixmap& _sourcePixmap, qreal _blurRadius)
{
    if (_sourcePixmap.isNull()) {
        return QPixmap();
    }

    QImage sourceImage = _sourcePixmap.toImage();
    QImage blurredImage(_sourcePixmap.size(), QImage::Format_ARGB32_Premultiplied);
    blurredImage.fill(0);
    QPainter blurPainter(&blurredImage);
    qt_blurImage(&blurPainter, sourceImage, _blurRadius, true, false);
    blurPainter.end();
    return QPixmap::fromImage(blurredImage);
}

QPixmap ImageHelper::dropShadow(const QPixmap& _sourcePixmap, const QMarginsF& _shadowMargins,
                                qreal _blurRadius, const QColor& _color, bool _useCache)
{
    if (_sourcePixmap.isNull()) {
        return QPixmap();
    }

    //
    // Кэш теней
    //
    using CacheKey = QPair<int, QPair<int, QPair<int, QRgb>>>;
    static QCache<CacheKey, QPixmap> s_shadowsCache;

    //
    // Ищем тень в кэше
    //
    const CacheKey shadowKey{ _sourcePixmap.width(),
                              { _sourcePixmap.height(),
                                { qCeil(_blurRadius * 100.0), _color.rgba() } } };
    if (_useCache && s_shadowsCache.contains(shadowKey)) {
        return *s_shadowsCache[shadowKey];
    }

    //
    // Подготовим расширенное изображение с дополнительным местом под тень
    //
    const QSizeF deltaSize(_shadowMargins.left() + _shadowMargins.right(),
                           _shadowMargins.top() + _shadowMargins.bottom());
    QImage shadowedImage(_sourcePixmap.size() + deltaSize.toSize(),
                         QImage::Format_ARGB32_Premultiplied);
    shadowedImage.fill(0);
    //
    // ...  рисуем исходное изображение
    //
    QPainter painter(&shadowedImage);
    painter.drawPixmap(QPointF(deltaSize.width() / 2.0, deltaSize.height() / 2.0), _sourcePixmap);
    painter.end();

    //
    // Блюрим альфа канал
    //
    QImage blurredImage(shadowedImage.size(), QImage::Format_ARGB32_Premultiplied);
    blurredImage.fill(0);
    QPainter blurPainter(&blurredImage);
    qt_blurImage(&blurPainter, shadowedImage, _blurRadius, true, false);
    blurPainter.end();
    shadowedImage = blurredImage;

    //
    // Покрасим тень
    //
    painter.begin(&shadowedImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(shadowedImage.rect(), _color);
    painter.end();

    auto shadowedPixmap = QPixmap::fromImage(shadowedImage);
    if (_useCache) {
        s_shadowsCache.insert(shadowKey, new QPixmap(shadowedPixmap));
    }
    return shadowedPixmap;
}

void ImageHelper::drawRoundedImage(QPainter& _painter, const QRectF& _rect, const QPixmap& _image,
                                   qreal _roundingRadius, int _notRoundedEdge)
{
    if (_rect.isEmpty()) {
        return;
    }

    const auto lastPen = _painter.pen();
    const auto lastBrush = _painter.brush();
    const auto lastAntialiasingState = _painter.testRenderHint(QPainter::Antialiasing);

    _painter.setPen(Qt::NoPen);
    QBrush imageBrush(_image);
    auto transform = imageBrush.transform();
    transform.translate(_rect.left(), _rect.top());
    imageBrush.setTransform(transform);
    _painter.setBrush(imageBrush);

    auto roundedRect = _rect;
    if (_notRoundedEdge != 0) {
        int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
        int edge = Qt::TopEdge;
        for (auto diff : { &dy1, &dx1, &dx2, &dy2 }) {
            if (_notRoundedEdge & edge) {
                // удвоение даёт достаточный запас, когда ширина или высота меньше радиуса
                *diff = _roundingRadius * 2;
            }
            edge <<= 1;
        }
        roundedRect.adjust(-dx1, -dy1, dx2, dy2);
        _painter.setClipRect(_rect);
    }

    _painter.setRenderHint(QPainter::Antialiasing);
    _painter.drawRoundedRect(roundedRect, _roundingRadius, _roundingRadius);

    //
    // Восстанавливаем состояние рисовальщика
    //
    _painter.setPen(lastPen);
    _painter.setBrush(lastBrush);
    _painter.setRenderHint(QPainter::Antialiasing, lastAntialiasingState);
}

QPixmap ImageHelper::rotateImage(const QPixmap& _image, bool _left)
{
    QPixmap rotatedImage(_image.height(), _image.width());

    QPainter painter(&rotatedImage);
    if (_left) {
        painter.translate(rotatedImage.rect().bottomLeft());
        painter.rotate(-90);
    } else {
        painter.translate(rotatedImage.rect().topRight());
        painter.rotate(90);
    }
    painter.drawPixmap(_image.rect(), _image);
    painter.end();

    return rotatedImage;
}

QSize ImageHelper::maxSize()
{
    return QSize(kImageMaxWidth, kImageMaxHeight);
}
