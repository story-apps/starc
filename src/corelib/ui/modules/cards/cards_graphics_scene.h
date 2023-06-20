#pragma once

#include <QGraphicsScene>


namespace Ui {

class AbstractCardItem;

class CardsGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CardsGraphicsScene(QObject* _parent = nullptr);
    ~CardsGraphicsScene() override;

    /**
     * @brief Возможно ли редактировать контент
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

    /**
     * @brief Скорректировать размер, чтобы влезли все элементы
     */
    void fitToContents();

    /**
     * @brief Получить следующее z-значение для позиционирования элемента наверху
     */
    qreal firstZValue() const;
    qreal nextZValue() const;

    /**
     * @brief Список перемещаемых в данный момент карточек
     */
    const QSet<AbstractCardItem*>& mouseGrabberItems() const;

signals:
    /**
     * @brief Пользователь сделал двойной клик на элементе
     */
    void itemDoubleClicked(const QModelIndex& _index);

    /**
     * @brief Пользователь изменил параметры акта
     */
    void itemChanged(const QModelIndex& _index);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
