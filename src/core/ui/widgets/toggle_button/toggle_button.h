#pragma once

#include <ui/widgets/widget/widget.h>

#include <QCloseEvent>


/**
 * @brief Кнопка переключатель
 */
class ToggleButton : public Widget
{
    Q_OBJECT

public:
    explicit ToggleButton(QWidget* _parent = nullptr);
    ~ToggleButton() override;

    /**
     * @brief Включён ли переключатель
     */
    bool isChecked() const;
    void setChecked(bool _checked);

    /**
     * @brief Задать иконку
     */
    void setIcon(const QString& _icon);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Изменилось состояние кнопки
     */
    void checkedChanged(bool _checked);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализуем включение переключателя при клике на нём
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
