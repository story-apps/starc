#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <QObject>

class QQmlContext;


/**
 * @brief Менеджер приложения
 */
class ApplicationManager : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationManager(QObject* _parent = nullptr);
    ~ApplicationManager() override;

    /**
     * @brief Настроить главный контекст приложения
     */
    void setupContext(QQmlContext* _rootContext);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

#endif // APPLICATION_MANAGER_H
