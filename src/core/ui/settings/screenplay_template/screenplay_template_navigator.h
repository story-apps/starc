#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Навигатор параметров шаблона сценария
 */
class ScreenplayTemplateNavigator : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTemplateNavigator(QWidget* _parent = nullptr);
    ~ScreenplayTemplateNavigator() override;

signals:
    /**
     * @brief Пользователь выбрал метрическую систему в которой он хочет видеть параметры шаблона
     */
    void mmCheckedChanged(bool _checked);

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
