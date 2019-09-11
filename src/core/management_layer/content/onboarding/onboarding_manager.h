#pragma once

#include <QLocale>
#include <QObject>

namespace Ui {
    enum class ApplicationTheme;
}


namespace ManagementLayer
{

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

    /**
     * @brief Пользователь выбрал тему приложения
     */
    void themeChanged(Ui::ApplicationTheme _theme);

    /**
     * @brief Пользователь изменил масштаб интерфейса
     */
    void scaleFactorChanged(qreal _scaleFactor);

    /**
     * @brief Посадка окончена
     */
    void finished();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
