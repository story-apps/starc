#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui
{

/**
 * @brief Представление настроек
 */
class SettingsView : public Widget
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget* _parent = nullptr);
    ~SettingsView() override;

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
