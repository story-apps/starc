#include "debouncer.h"


Debouncer::Debouncer(int _delayInMs, QObject* _parent)
    : QObject(_parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(_delayInMs);

    QObject::connect(&m_timer, &QTimer::timeout, this, [this] { emit gotWork(); });
}

void Debouncer::setAcceptWorkOrders(bool _accept)
{
    m_isAcceptWorkOrders = _accept;
}

void Debouncer::setDelay(int _delayInMs)
{
    m_timer.setInterval(_delayInMs);
}

void Debouncer::orderWork()
{
    if (!m_isAcceptWorkOrders) {
        return;
    }
    if (!m_timer.isActive()) {
        emit pendingWork();
    }
    m_timer.start();
}

void Debouncer::forceWork()
{
    if (!hasPendingWork()) {
        return;
    }

    m_timer.stop();
    emit gotWork();
}

void Debouncer::abortWork()
{
    m_timer.stop();
}

bool Debouncer::hasPendingWork() const
{
    return m_timer.isActive();
}
