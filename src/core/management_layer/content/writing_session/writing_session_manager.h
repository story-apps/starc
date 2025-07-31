#pragma once

#include <domain/starcloud_api.h>

#include <QObject>

class QKeyEvent;


namespace ManagementLayer {

/**
 * @brief Управляющий сессиями письма пользователя
 */
class WritingSessionManager : public QObject
{
    Q_OBJECT

public:
    explicit WritingSessionManager(QObject* _parent, QWidget* _parentWidget);
    ~WritingSessionManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Добавить событие нажатия кнопки
     */
    void addKeyPressEvent(QKeyEvent* _event);

    /**
     * @brief Дата и время последней синхронизации сессий
     */
    QDateTime sessionStatisticsLastSyncDateTime() const;

    /**
     * @brief Запустить сессию работы с проектом
     */
    void startSession(const QUuid& _projectUuid, const QString& _projectName);

    /**
     * @brief Включить/выключить подсчёт статистики
     * @note Используется при переключении между режимами чата/работы с текстом
     */
    void setCountingEnabled(bool _enabled);

    /**
     * @brief Разорвать текущую сессию в случае, когда пользователь долго бездействует
     */
    void splitSession(const QDateTime& _lastActionAt);

    /**
     * @brief Завершить текущую сессию работы с проектом
     */
    void finishSession();

    /**
     * @brief Показать панель писательского спринта
     */
    void showSprintPanel();

    /**
     * @brief Задать статистика по сессиям полученную с сервера
     */
    void setSessionStatistics(const QVector<Domain::SessionStatistics>& _sessionStatistics,
                              bool _ableToShowDeatils);

signals:
    /**
     * @brief Пользователь хочет выйти со страницы статистики
     */
    void closeSessionStatisticsRequested();

    /**
     * @brief Запрос на публикацию статистики по сессиям
     */
    void sessionStatisticsPublishRequested(
        const QVector<Domain::SessionStatistics>& _sessionStatistics);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
