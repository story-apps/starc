#pragma once

#include "card.h"

class QAbstractItemModel;


/**
 * @brief Базовая реализация карточки попапа
 */
class CORE_LIBRARY_EXPORT CardPopup : public Card
{
    Q_OBJECT

public:
    explicit CardPopup(QWidget* _parent = nullptr);
    ~CardPopup() override;

    /**
     * @brief Показать попап в заданном положении с заданным размером
     */
    void showPopup(const QPoint& _position, int _parentHeight);
    void showPopup(const QPoint& _position, int _parentHeight, const QSize& _size);

    /**
     * @brief Скрыть попап
     */
    void hidePopup();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
