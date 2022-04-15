#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет переключатель
 */
class CORE_LIBRARY_EXPORT Toggle : public Widget
{
    Q_OBJECT

public:
    explicit Toggle(QWidget* _parent = nullptr);
    ~Toggle() override;

    /**
     * @brief Зачекан ли переключатель
     */
    bool isChecked() const;
    void setChecked(bool _checked);

    /**
     * @brief Минимальный размер виджета
     */
    QSize minimumSizeHint() const override;

signals:
    /**
     * @brief Изменилось текущее выбранное значение
     */
    void checkedChanged(bool _checked);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для обработки клика по элементу
     */
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
