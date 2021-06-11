#pragma once

#include <ui/design_system/design_system.h>
#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

enum class ApplicationTheme;

class ThemeDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit ThemeDialog(QWidget* _parent = nullptr);
    ~ThemeDialog() override;

    /**
     * @brief Задать текущую тему приложения
     */
    void setCurrentTheme(ApplicationTheme _theme);

signals:
    /**
     * @brief Пользователь изменил тему
     */
    void themeChanged(ApplicationTheme _theme);

    /**
     * @brief Пользователь изменил цвета кастомной темы
     */
    void customThemeColorsChanged(const DesignSystem::Color& _color);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
