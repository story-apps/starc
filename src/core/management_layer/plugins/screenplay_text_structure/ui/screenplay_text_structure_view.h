#pragma once

#include <ui/abstract_navigator.h>

class QAbstractItemModel;


namespace Ui {

class ScreenplayTextStructureView : public AbstractNavigator
{
    Q_OBJECT

public:
    explicit ScreenplayTextStructureView(QWidget* _parent = nullptr);
    ~ScreenplayTextStructureView() override;

    /**
     * @brief Настроить навигатор в соответствии с параметрами заданными в настройках
     */
    void reconfigure();

    /**
     * @brief Задать заголовок навигатора
     */
    void setTitle(const QString& _title) override;

    /**
     * @brief Задать модель сцен сценария
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Установить текущий выделенный элемент в модели
     */
    void setCurrentModelIndex(const QModelIndex& _index);

signals:
    /**
     * @brief Пользователь выбрал элемент в навигаторе с заданным индексом
     */
    void currentModelIndexChanged(const QModelIndex& _index);

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
