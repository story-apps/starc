#pragma once

#include <ui/widgets/widget/widget.h>

namespace Ui {
enum class ApplicationTheme;
}


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
