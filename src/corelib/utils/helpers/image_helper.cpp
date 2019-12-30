#include "image_helper.h"

#include <QByteArray>
#include <QBuffer>
#include <QCache>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

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
}

QByteArray ImageHelper::bytesFromImage(const QPixmap& _image)
{
    //
    // Если необходимо корректируем размер изображения
    //
    QPixmap imageScaled = _image;
    if (imageScaled.width() > kImageMaxWidth
            || imageScaled.height() > kImageMaxHeight) {
        imageScaled = imageScaled.scaled(kImageMaxWidth, kImageMaxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
    const CacheKey imageKey{_svgPath, {_size.width(), _size.height()}};
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
    const CacheKey imageKey{_svgPath, {_size.width(), {_size.height(), _color.rgb()}}};
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

QPixmap ImageHelper::makeAvatar(const QString& _text, const QFont& _font, const QSize& _size, const QColor& _color)
{
    //
    // Кэш аватарок
    //
    using CacheKey = QPair<QString, QPair<int, QPair<int, QRgb>>>;
    static QCache<CacheKey, QPixmap> s_avatarsCache;

    //
    // Ищем аватарку в кэше
    //
    const CacheKey avatarKey{_text, {_size.width(), {_font.pixelSize(), _color.rgb()}}};
    if (s_avatarsCache.contains(avatarKey)) {
        return *s_avatarsCache[avatarKey];
    }

    //
    // Если нет в кэше, то рисуем
    //
    QImage avatar(_size, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    //
    // Формируем цвет для подложки заданного текста
    //
    ushort hash = 0;
    for (int characterIndex = 0; characterIndex < _text.length(); ++characterIndex) {
        hash += _text.at(characterIndex).unicode() + ((hash << 5) - hash);
    }
    hash = hash % 360;
    const QColor avatarColor = QColor::fromHsl(hash, 255 * 0.3, 255 * 0.6);

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
        const QStringList words = textToDraw.split(' ', QString::SkipEmptyParts);
        textToDraw = QString("%1%2").arg(words[0][0]).arg(words[1][0]);
    } else {
        textToDraw = textToDraw.left(2);
    }
    //
    // Рисуем текст
    //
    painter.setPen(_color);
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
    //
    // Кэш аватарок
    //
    using CacheKey = QPair<qint64, int>;
    static QCache<CacheKey, QPixmap> s_avatarsCache;

    //
    // Ищем аватарку в кэше
    //
    const CacheKey avatarKey{_pixmap.cacheKey(), _size.width()};
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
    circleClipPath.addEllipse(avatar.rect().adjusted(1,1,0,0).center(),
                              avatar.width()/2, avatar.height()/2);
    painter.setClipPath(circleClipPath);
    painter.drawPixmap(0, 0, _pixmap.scaled(_size,
                                            Qt::KeepAspectRatioByExpanding,
                                            Qt::SmoothTransformation));
    painter.end();

    //
    // И сохраняем в кэш для дальнейшего использования
    //
    QPixmap* avatarPixmap = new QPixmap(QPixmap::fromImage(avatar));
    s_avatarsCache.insert(avatarKey, avatarPixmap);
    return *avatarPixmap;
}

QPixmap ImageHelper::makeAvatar(const QPixmap& _pixmap, const QSize& _size, const QColor& _backgroundColor)
{
    //
    // Кэш аватарок
    //
    using CacheKey = QPair<qint64, QPair<int, QRgb>>;
    static QCache<CacheKey, QPixmap> s_avatarsCache;

    //
    // Ищем аватарку в кэше
    //
    const CacheKey avatarKey{_pixmap.cacheKey(), {_size.width(), _backgroundColor.rgba()}};
    if (s_avatarsCache.contains(avatarKey)) {
        return *s_avatarsCache[avatarKey];
    }

    //
    // Если нет в кэше, то рисуем
    //
    QImage avatar(_size, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

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
    const QRectF pixmapRect(QPointF(_size.width() - _pixmap.width(),
                                    _size.height() - _pixmap.height()) / 2.0,
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
extern void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

QPixmap ImageHelper::dropShadow(const QPixmap& _sourcePixmap, const QMarginsF& _shadowMargins, qreal _blurRadius, const QColor& _color)
{
    //
    // TODO: сделать кэш теней, т.к. это довольно долгая операция
    //

    if (_sourcePixmap.isNull()) {
        return QPixmap();
    }

    //
    // Подготовим расширенное изображение с дополнительным местом под тень
    //
    const QSizeF deltaSize(_shadowMargins.left() + _shadowMargins.right(),
                           _shadowMargins.top() + _shadowMargins.bottom());
    QImage shadowedImage(_sourcePixmap.size() + deltaSize.toSize(), QImage::Format_ARGB32_Premultiplied);
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

    return QPixmap::fromImage(shadowedImage);
}
