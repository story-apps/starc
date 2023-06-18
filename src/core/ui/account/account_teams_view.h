#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace Domain {
struct AccountInfo;
struct TeamInfo;
} // namespace Domain

namespace Ui {

class AccountTeamsView : public StackWidget
{
    Q_OBJECT

public:
    explicit AccountTeamsView(QWidget* _parent = nullptr);
    ~AccountTeamsView() override;

    /**
     * @brief Задать информацию об аккаунте пользователя
     */
    void setAccountInfo(const Domain::AccountInfo& _accountInfo);

    /**
     * @brief Задать список проектов
     */
    void setTeams(const QVector<Domain::TeamInfo>& _teams);

    /**
     * @brief Показать пустую страницу с возможностью, или без создавать новую команду
     */
    void showEmptyPage(bool _canCreateTeam);

    /**
     * @brief Показать параметры выбранной команды
     */
    void showTeam(int _teamId);

signals:
    /**
     * @brief Пользователь хочет добавить участника в коменду
     */
    void addMemberPressed(int _teamId, const QString& _email, const QString& _nameForTeam,
                          const QColor& _color);

    /**
     * @brief Пользователь хочет отписать соавтора от проекта
     */
    void removeMemberPressed(int _teamId, const QString& _email);

    /**
     * @brief Пользователь хочет открыть контекстное меню для соавтора
     */
    void collaboratorContextMenuRequested(const QString& _email, const QPoint& _position);

    /**
     * @brief Пользователь изменил роль соавтора
     */
    void changeRolePressed(const QString& _email, int _newRole);

    /**
     * @brief Пользователь изменил цвет соавтора
     */
    void changeColorPressed(const QString& _email, const QColor& _color);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
