#pragma once

#include <QObject>


namespace BusinessLayer {

/**
 * @brief Интерфейс класса для загрузки/сохранения изображений в БД
 */
class AbstractImageWrapper : public QObject
{
    Q_OBJECT

public:
    explicit AbstractImageWrapper(QObject* _parent = nullptr)
        : QObject(_parent)
    {
    }

    /**
     * @brief Получить изображение по заданному индентификатору
     */
    virtual QPixmap load(const QUuid& _uuid) const = 0;

    /**
     * @brief Установить изображение
     */
    virtual QUuid save(const QPixmap& _image) = 0;

    /**
     * @brief Сохранить изображение к существующему гуиду, полученное из внешнего сервиса
     */
    virtual void save(const QUuid& _uuid, const QPixmap& _image) = 0;
    virtual void save(const QUuid& _uuid, const QByteArray& _imageData) = 0;

signals:
    /**
     * @brief Добавлено новое изображение
     */
    void imageAdded(const QUuid& _uuid);

    /**
     * @brief Запрос изображения у внешнего поставщика изображений
     */
    void imageRequested(const QUuid& _uuid);

    /**
     * @brief Изображение было обновлено
     */
    void imageUpdated(const QUuid& _uuid, const QPixmap& _image);
};

} // namespace BusinessLayer
