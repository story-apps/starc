#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui
{

/**
 * @brief Навигатор по структуре проекта
 */
class ProjectNavigator : public Widget
{
    Q_OBJECT

public:
    explicit ProjectNavigator(QWidget* _parent = nullptr);
    ~ProjectNavigator() override;

    /**
     * @brief Задать модель документов проекта
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Сохранить состояние
     */
    QVariant saveState() const;

    /**
     * @brief Восстановить состояние
     */
    void restoreState(const QVariant& _state);

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

}
