#pragma once

#include <QLocale>
#include <QObject>


namespace ManagementLayer
{

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject* _parent, QWidget* _parentWidget);
    ~SettingsManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

signals:
    /**
     * @brief Пользователь хочет закрыть настройки
     */
    void closeSettingsRequested();

    /**
     * @brief Пользователь изменил язык
     */
    void languageChanged(QLocale::Language _language);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
