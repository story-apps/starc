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
     * @brief Валиден ли отчёт
     */
    virtual bool isValid() const
    {
        return false;
    }

    /**
     * @brief Сформировать отчёт из заданной модели
     */
    virtual void build(QAbstractItemModel* _model) = 0;

    /**
     * @brief Сохранить отчёт в файл
     */
    void saveToFile(const QString& _filename) const;

protected:
    /**
     * @brief Сохранить отчёт в файл конкретного формата
     */
    virtual void saveToPdf(const QString& _fileName) const
    {
    }
    virtual void saveToXlsx(const QString& _fileName) const = 0;
};

} // namespace BusinessLayer
