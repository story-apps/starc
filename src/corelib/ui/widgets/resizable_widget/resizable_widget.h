#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет, который анимирует свой размер и положение при изменении контента
 */
class CORE_LIBRARY_EXPORT ResizableWidget : public Widget
{
    Q_OBJECT

public:
    explicit ResizableWidget(QWidget* _parent = nullptr);
    ~ResizableWidget() override;

    /**
     * @brief Установить активность анимирования контента
     */
    void setResizingActive(bool _active);

protected:
    /**
     * @brief Переопределяем для реализации анимированного изменения размера
     */
    void resizeEvent(QResizeEvent* _event) override;
    void moveEvent(QMoveEvent* _event) override;

    /**
     * @brief Скругляем края
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
