#pragma once

#include <QTimer>

#include <corelib_global.h>


/**
 * @brief Класс для введения задержки между запросом некоторого действия и реализацией самого
 *        действия К примеру, для избежания флуда пользователями интерфейса
 */
class CORE_LIBRARY_EXPORT Debouncer : public QObject
{
    Q_OBJECT

public:
    /**
     * @param _delayInMs Задержка между последним вызовом orderWork и сигналом gotWork
     */
    explicit Debouncer(int _delayInMs, QObject* _parent = nullptr);

    /**
     * @brief Задать возможность принятия новых действий
     */
    void setAcceptWorkOrders(bool _accept);

    /**
     * @brief Задать задержку
     */
    void setDelay(int _delayInMs);

    /**
     * @brief Заказать некоторое действие, этот слот принимает сигналы от пользователя
     */
    void orderWork();

    /**
     * @brief Выполнить заказанное действие не дожидаясь завершения таймера
     */
    void forceWork();

    /**
     * @brief Прервать ожидание заказанного действия
     */
    void abortWork();

    /**
     * @brief Есть ли запланированная работа
     */
    bool hasPendingWork() const;

signals:
    /**
     * @brief Испускается, когда в первый раз вызвался orderWork, чтобы уведомить о том,
     *        что ожидается выполнение действия
     */
    void pendingWork();

    /**
     * @brief Когда пользователь успокоился (в рамках delayInMs), то испускается этот сигнал
     *        для того, чтобы сделать действие
     */
    void gotWork();

private:
    /**
     * @brief Принимаются ли заказы на новые действия
     */
    bool m_isAcceptWorkOrders = true;

    /**
     * @brief Таймер для осуществления задержки
     */
    QTimer m_timer;
};
