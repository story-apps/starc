#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Вспомогательный класс для работы со восьмушками
 */
class CORE_LIBRARY_EXPORT EighthsHelper
{
public:
    static QString toString(qreal _eighths, bool _withPostfix = false);
    static QString toStringWithPostfix(qreal _eighths);
};
