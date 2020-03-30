#pragma once

#include "identifier.h"

#include <QAbstractItemModel>


namespace Domain
{

/**
 * @brief Базовый класс для объектов извлекаемых из базы данных
 */
class CORE_LIBRARY_EXPORT DomainObject
{
public:
    DomainObject() = default;
    explicit DomainObject(const Identifier& _id);
    virtual ~DomainObject() = default;

public:
    Identifier id() const;
    void setId(const Identifier& _id);

    /**
     * @brief Сохранены ли изменения объекта
     */
    bool isChangesStored() const;

    /**
     * @brief Изменения сохранены
     */
    void markChangesStored();

    /**
     * @brief Изменения не сохранены
     */
    void markChangesNotStored();

private:
    /**
     * @brief Идентификатор объекта
     */
    Identifier m_id;

    /**
     * @brief Флаг изменений объекта
     */
    bool m_isChangesStored = false;
};

// ****

/**
 * @brief Интерфейс класса для загрузки/сохранения изображений в БД
 */
class AbstractImageWrapper {
public:
    virtual ~AbstractImageWrapper() = default;

    /**
     * @brief Получить изображение для заданного объекта
     */
    virtual QPixmap image(const DomainObject* _forObject) const = 0;

    /**
     * @brief Установить изображение для заданного объекта
     */
    virtual void setImage(const QPixmap& _image, const DomainObject* _forObject) = 0;
};

} // namespace Domain
