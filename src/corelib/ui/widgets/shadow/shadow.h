#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет тени, использующийся для декорирования основных виджетов
 */
class Shadow : public Widget
{
    Q_OBJECT

public:
    explicit Shadow(QWidget* _parent = nullptr);

protected:
    /**
     * @brief Переопределяем для отрисовки тени
     */
    void paintEvent(QPaintEvent* _event) override;
};
