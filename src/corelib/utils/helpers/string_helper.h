#pragma once

#include <qnamespace.h>


/**
 * @brief Функции конвертации перечислений в строки
 */
QString toString(Qt::Alignment _alignment);

/**
 * @brief Функции конвертации из строки в перечисление
 */
Qt::Alignment alignmentFromString(const QString& _text);
