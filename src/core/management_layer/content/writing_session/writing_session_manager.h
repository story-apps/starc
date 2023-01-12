#pragma once

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

    /**
     * @brief Добавить событие нажатия кнопки
     */
    void addKeyPressEvent(QKeyEvent* _event);

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
     * @brief Завершить текущую сессию работы с проектом
     */
    void finishSession();

    /**
     * @brief Показать панель писательского спринта
     */
    void showSprintPanel();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
