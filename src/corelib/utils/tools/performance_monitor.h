#pragma once

#include <QElapsedTimer>
#include <QString>

#include <corelib_global.h>


/**
 * @brief Measures an operation and logs its status when it takes too long
 */
class CORE_LIBRARY_EXPORT PerformanceMonitor
{
public:
    /**
     * @brief Start measuring an operation with a fallback status
     */
    explicit PerformanceMonitor(const QString& _status);
    ~PerformanceMonitor();

    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    /**
     * @brief Finish the measurement and log the status if the shared threshold was reached
     * @note When status is empty, the status passed to the constructor is used
     */
    void finish(const QString& _status = {});

    /**
     * @brief Time since the previous checkpoint, or since construction for the first checkpoint
     */
    qint64 checkpoint();

private:
    QElapsedTimer m_timer;
    QString m_status;
    qint64 m_lastCheckpoint = 0;
};
