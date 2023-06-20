#pragma once

#include "abstract_card_item.h"


namespace Ui {

/**
 * @brief Карточка
 */
class CardItem : public AbstractCardItem
{
public:
    explicit CardItem(QGraphicsItem* _parent = nullptr);
    ~CardItem() override;

    /**
     * @brief Определяем тип карточки, для быстрых кастов графических элементов
     */
    enum { Type = UserType + 1 };
    int type() const override;

    /**
     * @brief Отрисовка карточки
     */
    void paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option,
               QWidget* _widget) override;

    /**
     * @brief Анимируем hover
     */
    void hoverEnterEvent(QGraphicsSceneHoverEvent* _event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* _event) override;

    /**
     * @brief Реализуем клик на объекте
     */
    void mousePressEvent(QGraphicsSceneMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
