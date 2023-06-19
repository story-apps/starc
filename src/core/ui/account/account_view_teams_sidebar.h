#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace Domain {
struct TeamMemberInfo;
}


namespace Ui {

class AccountViewTeamsSidebar : public StackWidget
{
    Q_OBJECT

public:
    explicit AccountViewTeamsSidebar(QWidget* _parent = nullptr);
    ~AccountViewTeamsSidebar() override;

    /**
     * @brief Показать пустую страницу
     */
    void showEmptyPage();

    /**
     * @brief Задать участника команды, для отображения его параметров
     */
    void setTeamMember(const Domain::TeamMemberInfo& _member);

signals:
    /**
     * @brief Параметры участника изменены
     */
    void teamMemberChanged(const QString& _email, const QString& _nameForTeam, int _role,
                           bool _hasAccessToAllProjects, bool _allowGrantAccessToProjects);

protected:
    /**
     * @brief Обновляем переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем внешний вид
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
