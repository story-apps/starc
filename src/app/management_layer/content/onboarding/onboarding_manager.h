#ifndef ONBOARDING_MANAGER_H
#define ONBOARDING_MANAGER_H

#include <QObject>

class QQmlContext;


/**
 * @brief Менеджер посадочного экрана
 */
class OnboardingManager : public QObject
{
    Q_OBJECT

public:
    static QString toolBar();
    static QString navigator();
    static QString view();

    explicit OnboardingManager(QObject* _parent = nullptr);
    ~OnboardingManager() override;

    /**
     * @brief Настроить главный контекст приложения
     */
    void setupContext(QQmlContext* _rootContext);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

#endif // ONBOARDING_MANAGER_H
