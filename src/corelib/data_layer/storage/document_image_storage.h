#pragma once

#include <business_layer/model/abstract_image_wrapper.h>

#include <QObject>

#include <corelib_global.h>


namespace DataStorageLayer {

/**
 * @brief Хранилище документов-изображений
 */
class CORE_LIBRARY_EXPORT DocumentImageStorage : public QObject,
                                                 public BusinessLayer::AbstractImageWrapper
{
    Q_OBJECT

public:
    explicit DocumentImageStorage(QObject* _parent = nullptr);
    ~DocumentImageStorage() override;

    /**
     * @brief Получить изображение с заданным гуидом
     *        - из кэша
     *        - загрузить из базы
     *        - если нигде нет, то запросить у внешнего сервиса
     */
    QPixmap load(const QUuid& _uuid) const override;

    /**
     * @brief Сохранить новое изображение
     */
    QUuid save(const QPixmap& _image) override;

    /**
     * @brief Сохранить изображение к существующему гуиду, полученное из внешнего сервиса
     */
    void save(const QUuid& _uuid, const QByteArray& _imageData);

    /**
     * @brief Очистить хранилище
     */
    void clear();

    /**
     * @brief Сохранить все новые изображения, ещё не сохранённые в базу данных
     */
    void saveChanges();

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer
