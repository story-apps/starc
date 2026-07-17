#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Вспомогательный класс для работы со восьмушками
 */
class CORE_LIBRARY_EXPORT EightsHelper
{
public:
    static QString toString(qreal _eights, bool _withPostfix = false);
    static QString toStringWithPostfix(qreal _eights);
};
