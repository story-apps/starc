#pragma once

#include <corelib_global.h>

class QAbstractItemModel;

namespace BusinessLayer {

/**
 * @brief Базовый класс для отчётов
 */
class CORE_LIBRARY_EXPORT AbstractReport
{
public:
    virtual ~AbstractReport() = default;

    /**
     * @brief Сформировать отчёт из заданной модели
     */
    virtual void build(QAbstractItemModel* _model) = 0;
};

} // namespace BusinessLayer
