#include "performance_monitor.h"

#include <utils/logging.h>


namespace {
constexpr qint64 kSlowOperationThresholdMs = 80;
}


PerformanceMonitor::PerformanceMonitor(const QString& _status)
    : m_status(_status)
{
    m_timer.start();
}

PerformanceMonitor::~PerformanceMonitor()
{
    finish();
}

void PerformanceMonitor::finish(const QString& _status)
{
    if (!m_timer.isValid()) {
        return;
    }

    const auto elapsed = m_timer.elapsed();
    m_timer.invalidate();
    if (elapsed < kSlowOperationThresholdMs) {
        return;
    }

    Log::warning("[PerformanceMonitor] Slow operation detected (%1 ms): %2",
                 QString::number(elapsed), !_status.isEmpty() ? _status : m_status);
}

qint64 PerformanceMonitor::checkpoint()
{
    if (!m_timer.isValid()) {
        return 0;
    }

    const auto currentCheckpoint = m_timer.elapsed();
    const auto checkpointDuration = currentCheckpoint - m_lastCheckpoint;
    m_lastCheckpoint = currentCheckpoint;
    return checkpointDuration;
}
