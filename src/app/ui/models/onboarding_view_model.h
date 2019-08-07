#pragma once

#include <QObject>


/**
 * @brief Модель посадочного экрана
 */
class OnboardingViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int stepIndex READ stepIndex WRITE setStepIndex NOTIFY stepIndexChanged)

public:
    explicit OnboardingViewModel(QObject* _parent = nullptr);
    ~OnboardingViewModel() override;

    /**
     * @brief Номер текущего шага
     */
    int stepIndex() const;
    void setStepIndex(int _index);
    Q_SIGNAL void stepIndexChanged(int _index);

    /**
     * @brief Завершить посадку
     */
    void finish();

signals:
    /**
     * @brief Посадка окончена
     */
    void onboardingFinished();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
