#include "run_once.h"

RunOnceLock::RunOnceLock()
{
}

RunOnceLock::RunOnceLock(const QString& _key)
    : m_key(_key)
{
}

RunOnceLock::~RunOnceLock()
{
    RunOnce::s_lockedKeys.remove(m_key);
}


const RunOnceLock RunOnce::tryRun(const QString& _key)
{
    if (isRunned(_key))
        return RunOnceLock();

    s_lockedKeys.insert(_key);
    return RunOnceLock(_key);
}

bool RunOnce::isRunned(const QString& _key)
{
    return s_lockedKeys.contains(_key);
}

bool RunOnce::canRun(const QString& _key)
{
    return !isRunned(_key);
}

QSet<QString> RunOnce::s_lockedKeys;
