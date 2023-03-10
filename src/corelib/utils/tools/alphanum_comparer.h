#pragma once

#include <QString>

#include <corelib_global.h>


/*!
 *    \brief      Natural (alpha-num) sorting
 *    \author     Litkevich Yuriy
 *    \see http://www.forum.crossplatform.ru/index.php?showtopic=6244&st=0&p=44752&#entry44752
 */
class CORE_LIBRARY_EXPORT AlphanumComparer
{
public:
    /**
     * @brief Сравнить две строки по правилам натуральной сортировки
     */
    static bool lessThan(const QString& s1, const QString& s2);
};
