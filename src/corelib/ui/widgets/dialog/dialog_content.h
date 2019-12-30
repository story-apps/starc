#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет содержимого диалога, который анимирует свой размер при изменении контента
 */
class DialogContent : public Widget
{
    Q_OBJECT

public:
    explicit DialogContent(QWidget* _parent = nullptr);
    ~DialogContent() override;

protected:
    /**
     * @brief Переопределяем для реализации анимированного изменения размера
     */
    void resizeEvent(QResizeEvent* _event) override;
    void moveEvent(QMoveEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
