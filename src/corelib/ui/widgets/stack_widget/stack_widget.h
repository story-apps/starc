#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет реализующий стэк вложенных виджетов, один из которых является видимым
 */
class StackWidget : public Widget
{
    Q_OBJECT

public:
    explicit StackWidget(QWidget* _parent = nullptr);
    ~StackWidget() override;

    /**
     * @brief Сделать заданный виджет текущим
     */
    void setCurrentWidget(QWidget* widget);

protected:
    /**
     * @brief Реализуем собственную отрисовку для реализации эффекста смены текущего виджета
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализацем собственную отрисовку, чтобы корректировать размер вложенных виджетов
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
