#pragma once

#include <QObject>

namespace ManagementLayer
{

/**
 * @brief Менеджер личного кабинета пользователя
 */
class AccountManager : public QObject
{
    Q_OBJECT

public:
    explicit AccountManager(QObject* _parent, QWidget* _parentWidget);
    ~AccountManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Авторизовать в личном кабинете
     */
    void authorize();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
