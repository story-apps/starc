#pragma once

#include <corelib_global.h>

#include <QSet>


/**
 * @brief Замок для одиночного запуска
 */
class CORE_LIBRARY_EXPORT RunOnceLock
{
public:
    ~RunOnceLock();

    inline operator bool() const { return !m_key.isNull(); }

private:
    /**
     * @brief Ключ доступа к замку
     */
    QString m_key;

    RunOnceLock();
    explicit RunOnceLock(const QString& _key);

    /**
     * Дружим с классом RunOnce, чтобы он мог конструировать объекты RunOnceLock
     */
    friend class RunOnce;
};

/**
 * @brief Фасад для единовременного запуска функций
 */
class CORE_LIBRARY_EXPORT RunOnce
{
    RunOnce() = delete;

public:
    /**
     * @brief Попробовать захватить запуск для заданного ключа
     */
    static const RunOnceLock tryRun(const QString& _key);

    /**
     * @brief Захвачен ли запуск для заданного ключа
     */
    static bool isRunned(const QString& _key);

    /**
     * @brief Можно ли захватить запуск для заданного ключа
     */
    static bool canRun(const QString& _key);

private:
    /**
     * @brief Ключи от заблокированных замков
     */
    static QSet<QString> s_lockedKeys;

    /**
     * Дружим с классом RunOnceLock для доступа к списку ключей
     */
    friend class RunOnceLock;
};

