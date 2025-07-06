#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>

#include <chrono>

namespace BusinessLayer {

/**
 * @brief Сводный отчёт по аудиопостановке
 */
class CORE_LIBRARY_EXPORT AudioplaySummaryReport : public AbstractReport
{
public:
    AudioplaySummaryReport();
    ~AudioplaySummaryReport() override;

    /**
     * @brief Валиден ли отчёт
     */
    bool isValid() const override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Длительность аудиопостановки
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Количество страниц
     */
    int pagesCount() const;

    /**
     * @brief Количество слов
     */
    int wordsCount() const;

    /**
     * @brief Количество символов
     */
    /** @{ */
    struct CharactersCount {
        int withoutSpaces = 0;
        int withSpaces = 0;
    };
    CharactersCount charactersCount() const;
    /** @} */

    /**
     * @brief Получить информацию о тексте
     */
    QAbstractItemModel* textInfoModel() const;

    /**
     * @brief Получить информацию о персонажах
     */
    QAbstractItemModel* charactersInfoModel() const;

protected:
    /**
     * @brief Сохранить отчёт в файл
     */
    void saveToPdf(const QString& _fileName) const override;
    void saveToXlsx(const QString& _fileName) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
