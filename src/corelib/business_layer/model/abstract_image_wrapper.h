#pragma once

class QPixmap;
class QUuid;


namespace BusinessLayer {

/**
 * @brief Интерфейс класса для загрузки/сохранения изображений в БД
 */
class AbstractImageWrapper
{
public:
    virtual ~AbstractImageWrapper() {}

    /**
     * @brief Получить изображение по заданному индентификатору
     */
    virtual QPixmap load(const QUuid& _uuid) const = 0;

    /**
     * @brief Установить изображение
     */
    virtual QUuid save(const QPixmap& _image) = 0;
};

} // namespace BusinessLayer
