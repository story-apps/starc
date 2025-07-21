#pragma once

#include <ui/modules/cards/abstract_card_item.h>

namespace BusinessLayer {
class ProjectsModelProjectItem;
}


namespace Ui {

/**
 * @brief Карточка проекта
 */
class ProjectCard : public AbstractCardItem
{
public:
    explicit ProjectCard(QGraphicsItem* _parent = nullptr);
    ~ProjectCard() override;

    /**
     * @brief Определяем тип карточки, для быстрых кастов графических элементов
     */
    enum { Type = UserType + 10 };
    int type() const override;

    /**
     * @brief Проект
     */
    BusinessLayer::ProjectsModelProjectItem* project() const;

    /**
     * @brief Может ли карточка быть расположена после заданной
     */
    bool canBePlacedAfter(AbstractCardItem* _previousCard) override;

    /**
     * @brief Может ли карточка быть вложена в контейнер
     */
    bool canBeEmbedded(AbstractCardItem* _container) const override;

protected:
    /**
     * @brief Инициилизировать карточку после установки элемента модели в неё
     */
    void init() override;

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
