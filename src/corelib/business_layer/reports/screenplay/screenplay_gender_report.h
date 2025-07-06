#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по гендерной аналитике
 */
class CORE_LIBRARY_EXPORT ScreenplayGenderReport : public AbstractReport
{
public:
    ScreenplayGenderReport();
    ~ScreenplayGenderReport() override;

    /**
     * @brief Валиден ли отчёт
     */
    bool isValid() const override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Количество прохождений текст Бекдел
     */
    int bechdelTest() const;

    /**
     * @brief Количество прохождений реверсивного теста Бекдел
     */
    int reverseBechdelTest() const;

    /**
     * @brief Получить информацию о сценах
     */
    QAbstractItemModel* scenesInfoModel() const;

    /**
     * @brief Получить информацию о диалогах
     */
    QAbstractItemModel* dialoguesInfoModel() const;

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
