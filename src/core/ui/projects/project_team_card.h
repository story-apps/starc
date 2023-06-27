#pragma once

#include <ui/modules/cards/abstract_card_item.h>


namespace Ui {

/**
 * @brief Карточка команды проектов
 */
class ProjectTeamCard : public AbstractCardItem
{
public:
    explicit ProjectTeamCard(QGraphicsItem* _parent = nullptr);

    /**
     * @brief Определяем тип карточки, для быстрых кастов графических элементов
     */
    enum { Type = UserType + 11 };
    int type() const override;

    /**
     * @brief Высота заголовка
     */
    qreal headerHeight() const override;

    /**
     * @brief Является ли карточка контейнером
     */
    bool isContainer() const override;

    /**
     * @brief Открыта ли карточка
     */
    bool isOpened() const override;

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
     * @brief Реализуем клик на объекте
     */
    void mousePressEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* _event) override;

    /**
     * @brief При двойном клике разворачиваем/сворачиваем группу
     */
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
