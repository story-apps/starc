#include "run_once.h"


namespace {

/**
 * @brief Ключи заблокированных замков — свои для каждого потока
 */
QSet<QString>& lockedKeys()
{
    thread_local QSet<QString> keys;
    return keys;
}

} // namespace

RunOnceLock::RunOnceLock(const QString& _key)
    : m_key(_key)
{
}

RunOnceLock::RunOnceLock(RunOnceLock&& _other) noexcept
    : m_key(std::move(_other.m_key))
{
    _other.m_key = QString(); // чтобы деструктор источника не снял блокировку
}

RunOnceLock::~RunOnceLock()
{
    if (!m_key.isNull()) {
        lockedKeys().remove(m_key);
    }
}


const RunOnceLock RunOnce::tryRun(const QString& _key)
{
    if (isRunned(_key)) {
        return {};
    }

    lockedKeys().insert(_key);
    return RunOnceLock(_key);
}

bool RunOnce::isRunned(const QString& _key)
{
    return lockedKeys().contains(_key);
}

bool RunOnce::canRun(const QString& _key)
{
    return !isRunned(_key);
}
