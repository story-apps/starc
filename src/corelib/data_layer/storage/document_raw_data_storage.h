#pragma once

#include <business_layer/model/abstract_raw_data_wrapper.h>

#include <corelib_global.h>


namespace DataStorageLayer {

/**
 * @brief Хранилище документов сырых данных
 */
class CORE_LIBRARY_EXPORT DocumentRawDataStorage : public BusinessLayer::AbstractRawDataWrapper
{
    Q_OBJECT

public:
    DocumentRawDataStorage(QObject* _parent = nullptr);
    ~DocumentRawDataStorage() override;

    /**
     * @brief Получить данные по заданному индентификатору
     */
    QByteArray load(const QUuid& _uuid) const override;

    /**
     * @brief Сохранить данные
     */
    QUuid save(const QByteArray& _data) override;

    /**
     * @brief Сохранить данные к существующему гуиду, полученное из внешнего сервиса
     */
    void save(const QUuid& _uuid, const QByteArray& _data) override;

    /**
     * @brief Удалить данные
     */
    void remove(const QUuid& _uuid) override;

    /**
     * @brief Очистить хранилище
     */
    void clear();

    /**
     * @brief Сохранить все новые документы, ещё не сохранённые в базу данных
     */
    void saveChanges();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer
