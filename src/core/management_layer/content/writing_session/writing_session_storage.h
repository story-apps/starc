#pragma once

#include <QDateTime>
#include <QtContainerFwd>

namespace Domain {
struct SessionStatistics;
}


namespace DataStorageLayer {

/**
 * @brief Хранилище сессий работы с приложением
 */
class WritingSessionStorage
{
public:
    WritingSessionStorage();

    /**
     * @brief Дата и время последней синхронизации сессий
     */
    QDateTime sessionStatisticsLastSyncDateTime() const;

    /**
     * @brief Сохранить дату/время последней синхронизации сессий
     */
    void saveSessionStatisticsLastSyncDateTime(const QDateTime& _dateTime);

    /**
     * @brief Получить список сессий с заданной даты
     */
    QVector<Domain::SessionStatistics> sessionStatistics(const QDateTime& _fromDateTime = {}) const;

    /**
     * @brief Сохранить заданный список сессий
     */
    void saveSessionStatistics(const QVector<Domain::SessionStatistics>& _sessionStatistics);
};

} // namespace DataStorageLayer
