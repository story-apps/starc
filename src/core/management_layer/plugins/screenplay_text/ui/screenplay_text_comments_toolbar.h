#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui
{

/**
 * @brief Панель инструментов рецензирования
 */
class ScreenplayTextCommentsToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ScreenplayTextCommentsToolbar(QWidget* _parent = nullptr);
    ~ScreenplayTextCommentsToolbar() override;

    /**
     * @brief Отобразить диалог
     */
    void showToolbar();

    /**
     * @brief Скрыть диалог
     */
    void hideToolbar();

signals:

protected:
    void paintEvent(QPaintEvent* _event) override;

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
