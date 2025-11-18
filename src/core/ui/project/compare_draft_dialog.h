#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

class CompareDraftDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CompareDraftDialog(QWidget* _parent = nullptr);
    ~CompareDraftDialog() override;

    /**
     * @brief Задать список драфтов из которых можно создать новую
     */
    void setDrafts(const QString& _lhsName, const QStringList& _lhsDrafts, int _selectLhsDraftIndex,
                   const QString& _rhsName, const QStringList& _rhsDrafts,
                   int _selectRhsDraftIndex);

signals:
    /**
     * @brief Пользователь нажал кнопку сравнения выбранных драфтов
     */
    void comparePressed(int _lhsDraftIndex, int _rhsDraftIndex);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
