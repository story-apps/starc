#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace BusinessLayer {
struct ComplianceCheckResult;
}


namespace Ui {

/**
 * @brief Представление списка результатов проверки требований к сценарию
 */
class ComplianceCheckResultView : public StackWidget
{
    Q_OBJECT

public:
    explicit ComplianceCheckResultView(QWidget* _parent = nullptr);
    ~ComplianceCheckResultView() override;

    /**
     * @brief Задать список с результатами проверок
     */
    void setCheckResults(const QVector<BusinessLayer::ComplianceCheckResult>& _results);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
