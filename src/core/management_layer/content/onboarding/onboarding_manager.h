#pragma once

#include <QLocale>
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

signals:
    /**
     * @brief Пользователь выбрал язык приложения
     */
    void languageChanged(QLocale::Language _language);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
