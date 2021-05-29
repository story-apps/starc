#pragma once

#include <ui/widgets/widget/widget.h>


class CORE_LIBRARY_EXPORT CheckBox : public Widget
{
    Q_OBJECT

public:
    explicit CheckBox(QWidget* _parent = nullptr);
    ~CheckBox() override;

    /**
     * @brief Включён ли переключатель
     */
    bool isChecked() const;
    void setChecked(bool _checked);

    /**
     * @brief Задать текст переключателя
     */
    void setText(const QString& _text);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Изменилось состояние включённости переключателя
     */
    void checkedChanged(bool _checked);

protected:
    /**
     * @brief Реализуем отрисовку
     */
    void paintEvent(QPaintEvent *_event) override;

    /**
     * @brief Реализуем включение переключателя при клике на нём
     */
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы нажатие пробела и энтера активировало кнопку
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

