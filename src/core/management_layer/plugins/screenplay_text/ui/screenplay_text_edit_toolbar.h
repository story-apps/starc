#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui
{

/**
 * @brief Панель инструментов редактора сценария
 */
class ScreenplayTextEditToolBar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ScreenplayTextEditToolBar(QWidget* _parent = nullptr);
    ~ScreenplayTextEditToolBar() override;

signals:
    void undoPressed();
    void redoPressed();

protected:
    void leaveEvent(QEvent* _event) override;

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
