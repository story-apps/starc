#pragma once

#include <corelib_global.h>

#include <QString>
#include <QHash>
#include <QVariant>


namespace Domain
{
/**
 * @brief Класс идентификатора объектов в базе данных
 */
class CORE_LIBRARY_EXPORT Identifier
{
public:
    Identifier();
    explicit Identifier(int _id, int _version = 0);

public:
    /**
     * @brief Получение следующего идентификатора
     */
    Identifier next() const;

    /**
     * @brief Получить идентификатор следующей версии
     */
    Identifier nextVersion() const;

public:
    /**
     * @brief Проверка корректности идентификатора
     */
    bool isValid() const;

    /**
     * @brief Значение идентификатора
     */
    int value() const;

    /**
     * @brief Версия идентификатора
     */
    int version() const;

private:
    int m_id = 0;
    int m_version = 0;
    bool m_isValid = false;
};

// ****
// Функции необходимые для возможности использования класса в контейнерах QMap и QHash

inline bool operator==(const Identifier& _id1, const Identifier& _id2)
{
    return _id1.value() == _id2.value()
           && _id1.version() == _id2.version();
}
inline bool operator!=(const Identifier& _id1, const Identifier& _id2)
{
    return !(_id1 == _id2);
}
inline bool operator>(const Identifier& _id1, const Identifier& _id2)
{
    if (_id1.value() != _id2.value()) {
        return _id1.value() > _id2.value();
    } else {
        return _id1.version() > _id2.version();
    }
}
inline bool operator<(const Identifier& _id1, const Identifier& _id2)
{
    if (_id1.value() != _id2.value()) {
        return _id1.value() < _id2.value();
    } else {
        return _id1.version() < _id2.version();
    }
}
inline uint qHash(const Identifier& _key)
{
    return qHash(QString("%1.%2").arg(_key.value()).arg(_key.version()));
}

} // namespace Domain

Q_DECLARE_METATYPE(Domain::Identifier)
