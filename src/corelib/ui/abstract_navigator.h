#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Интерфейс менеджера документа
 */
class CORE_LIBRARY_EXPORT AbstractNavigator : public Widget
{
    Q_OBJECT

public:
    explicit AbstractNavigator(QWidget* _parent = nullptr)
        : Widget(_parent)
    {
    }

    /**
     * @brief Задать заголовок навигатора
     */
    virtual void setTitle(const QString& _title) = 0;

signals:
    /**
     * @brief Пользователь хочет выйти из навигатора
     */
    void backPressed();
};

} // namespace Ui
