#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Интерфейс менеджера документа
 */
class AbstractNavigator : public Widget
{
    Q_OBJECT

public:
    explicit AbstractNavigator(QWidget* _parent = nullptr) : Widget(_parent) {}

signals:
    /**
     * @brief Пользователь хочет выйти из навигатора
     */
    void backPressed();
};

} // namespace Ui
