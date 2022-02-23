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
     * @brief Показать панель писательского спринта
     */
    void showSprintPanel();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
