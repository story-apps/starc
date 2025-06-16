#pragma once

#include <QObject>

#include <corelib_global.h>

namespace Domain {
enum class DocumentObjectType;
}

namespace BusinessLayer {

/**
 * @brief Интерфейс класса для загрузки/сохранения сырых данных в БД
 */
class CORE_LIBRARY_EXPORT AbstractRawDataWrapper : public QObject
{
    Q_OBJECT

public:
    explicit AbstractRawDataWrapper(QObject* _parent = nullptr)
        : QObject(_parent)
    {
    }

    virtual ~AbstractRawDataWrapper() = default;

    /**
     * @brief Получить данные по заданному индентификатору
     */
    virtual QByteArray load(const QUuid& _uuid) const = 0;

    /**
     * @brief Сохранить данные
     */
    virtual QUuid save(const QByteArray& _data) = 0;

    /**
     * @brief Удалить данные
     */
    virtual void remove(const QUuid& _uuid) = 0;
};

} // namespace BusinessLayer
