#pragma once

#include <qnamespace.h>

#include <corelib_global.h>

class QMarginsF;
class QRectF;


/**
 * @brief Функции конвертации из строки в перечисление
 */
CORE_LIBRARY_EXPORT Qt::Alignment alignmentFromString(const QString& _text);
CORE_LIBRARY_EXPORT QMarginsF marginsFromString(const QString& _margins);
CORE_LIBRARY_EXPORT QRectF rectFromString(const QString& _rect);

/**
 * @brief Преобразование разных типов в строку
 */
CORE_LIBRARY_EXPORT QString toString(bool _value);
CORE_LIBRARY_EXPORT QString toString(int _value);
CORE_LIBRARY_EXPORT QString toString(qreal _value);
CORE_LIBRARY_EXPORT QString toString(Qt::Alignment _alignment);
CORE_LIBRARY_EXPORT QString toString(const QMarginsF& _margins);
CORE_LIBRARY_EXPORT QString toString(const QRectF& _rect);
