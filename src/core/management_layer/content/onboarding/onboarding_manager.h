#pragma once

#include <QObject>


/**
 * @brief Менеджер посадочного экрана
 */
class OnboardingManager : public QObject
{
    Q_OBJECT

public:
    explicit OnboardingManager(QObject* _parent, QWidget* _parentWidget);
    ~OnboardingManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
