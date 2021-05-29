#pragma once

class QAbstractItemModel;

namespace BusinessLayer
{

/**
 * @brief Базовый класс для отчётов
 */
class AbstractReport
{
public:
    virtual ~AbstractReport() = default;

    /**
     * @brief Сформировать отчёт из заданной модели
     */
    virtual void build(QAbstractItemModel* _model) = 0;
};

} // namespace BusinessLayer
