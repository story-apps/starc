#pragma once

#include <QGraphicsScene>

#include <corelib_global.h>


namespace Ui {

class AbstractCardItem;

class CORE_LIBRARY_EXPORT CardsGraphicsScene : public QGraphicsScene
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
     * @brief Добавлять дополнительную прокрутку
     */
    void setAdditionalScrollingAvailable(bool _available);

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
     * @brief Пользователь сделал клик на элементе
     */
    void itemClicked(const QModelIndex& _index);

    /**
     * @brief Пользователь сделал двойной клик на элементе
     */
    void itemDoubleClicked(const QModelIndex& _index);

    /**
     * @brief Пользователь вызвал контекстное меню карточки
     */
    void itemContextMenuRequested(const QModelIndex& _index);

    /**
     * @brief Пользователь изменил карточку
     */
    void itemChanged(const QModelIndex& _index);

    /**
     * @brief Пользователь подвинул карточку
     */
    void itemMoved(const QModelIndex& _index);

    /**
     * @brief Пользователь отпустил карточку
     */
    void itemDropped(const QModelIndex& _index);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
