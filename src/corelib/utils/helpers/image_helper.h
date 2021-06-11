#pragma once

#include <QtGlobal>

#include <corelib_global.h>

class QByteArray;
class QColor;
class QFont;
class QIcon;
class QMarginsF;
class QPainter;
class QPixmap;
class QRectF;
class QSize;
class QString;


/**
 * @brief Вспомогательные функции для работы с изображениями
 */
class CORE_LIBRARY_EXPORT ImageHelper
{
public:
    /**
     * @brief Сохранение изображения в массив байт
     */
    static QByteArray bytesFromImage(const QPixmap& _image);

    /**
     * @brief Загрузить изображение из массива байт
     */
    static QPixmap imageFromBytes(const QByteArray& _bytes);

    /**
     * @brief Загрузить изображение из SVG в нужном размере
     */
    static QPixmap loadSvg(const QString& _svgPath, const QSize& _size);

    /**
     * @brief Загрузить изображение из SVG в нужном размере и заданным цветом
     */
    static QPixmap loadSvg(const QString& _svgPath, const QSize& _size, const QColor& _color);

    /**
     * @brief Установить цвет изображения
     */
    static void setPixmapColor(QPixmap& _pixmap, const QColor& _color);

    /**
     * @brief Сравнить два изображения
     */
    static bool isImagesEqual(const QPixmap& _lhs, const QPixmap& _rhs);

    /**
     * @brief Сделать аватар из заданного текста заданного размера
     */
    static QPixmap makeAvatar(const QString& _text, const QFont& _font, const QSize& _size,
                              const QColor& _color);

    /**
     * @brief Сделать аватар из заданной картинки
     */
    static QPixmap makeAvatar(const QPixmap& _pixmap, const QSize& _size);

    /**
     * @brief Поместить заданное изображение в круг заданного размера и цвета
     */
    static QPixmap makeAvatar(const QPixmap& _pixmap, const QSize& _size,
                              const QColor& _backgroundColor);

    /**
     * @brief Получить изображение тени для заданного изображения и параметров
     */
    static QPixmap dropShadow(const QPixmap& _sourcePixmap, const QMarginsF& _shadowMargins,
                              qreal _blurRadius, const QColor& _color, bool _useCache = false);

    /**
     * @brief Нарисовать изображение в заданной области со скруглёнными краями
     */
    static void drawRoundedImage(QPainter& _painter, const QRectF& _rect, const QPixmap& _image,
                                 qreal _roundingRadius, int _notRoundedEdge = 0);
};
