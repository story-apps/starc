#pragma once

#include <ui/widgets/widget/widget.h>


class Card : public Widget
{
    Q_OBJECT

public:
    explicit Card(QWidget* _parent = nullptr);
    ~Card() override;

protected:
    /**
     * @brief Переопределяем для реализации отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для реализации эффекта поднятия виджета при ховере
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
