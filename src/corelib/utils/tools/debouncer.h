#pragma once

#include <QTimer>

#include <corelib_global.h>


/**
 * @brief Класс для введения задержки между запросом некоторого действия и реализацией самого
 * действия К примеру, для избежания флуда пользователями интерфейса
 */
class CORE_LIBRARY_EXPORT Debouncer : public QObject
{
    Q_OBJECT

public:
    /**
     * @param _delayInMs Задержка между последним вызовом orderWork и сигналом gotWork
     */
    explicit Debouncer(int _delayInMs, QObject* _parent = nullptr);

public slots:
    /**
     * @brief Заказать некоторое действие, этот слот принимает сигналы от пользователя
     */
    void orderWork();

    /**
     * @brief Прервать ожидание заказанного действия
     */
    void abortWork();

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
    QTimer m_timer;
};
