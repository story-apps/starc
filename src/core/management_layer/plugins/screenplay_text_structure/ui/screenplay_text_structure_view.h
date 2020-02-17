#pragma once

#include <ui/abstract_navigator.h>

class QAbstractItemModel;


namespace Ui
{

class ScreenplayTextStructureView : public AbstractNavigator
{
    Q_OBJECT

public:
    explicit ScreenplayTextStructureView(QWidget* _parent = nullptr);
    ~ScreenplayTextStructureView() override;

    /**
     * @brief Задать заголовок навигатора
     */
    void setTitle(const QString& _title) override;

    /**
     * @brief Задать модель сцен сценария
     */
    void setModel(QAbstractItemModel* _model);

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
