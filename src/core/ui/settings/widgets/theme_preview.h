#pragma once

#include <ui/design_system/design_system.h>
#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет-Превьюшка для заданной темы приложения
 */
class ThemePreview : public Widget
{
    Q_OBJECT

public:
    explicit ThemePreview(QWidget* _parent = nullptr);
    ~ThemePreview() override;

    /**
     * @brief Задать тему для построения превьюшки
     */
    void setTheme(Ui::ApplicationTheme _theme);

    /**
     * @brief Используем фиксированный размер
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь кликнул на теме
     */
    void themePressed(Ui::ApplicationTheme _theme);

    /**
     * @brief Был вставлен хэш кастомной темы
     */
    void customThemeHashPasted(const Ui::DesignSystem::Color& _color);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Испускаем сигнал, что пользователь кликнул на теме
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Настраиваем отступы контента
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
