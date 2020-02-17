#pragma once

#include <qnamespace.h>

class QMarginsF;


/**
 * @brief Функции конвертации из строки в перечисление
 */
Qt::Alignment alignmentFromString(const QString& _text);
QMarginsF marginsFromString(const QString& _margins);

/**
 * @brief Преобразование разных типов в строку
 */
QString toString(bool _value);
QString toString(int _value);
QString toString(qreal _value);
QString toString(Qt::Alignment _alignment);
QString toString(const QMarginsF& _margins);
