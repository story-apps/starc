#pragma once

#include <ui/widgets/widget/widget.h>

#include <QCloseEvent>


/**
 * @brief Кнопка с иконкой
 */
class CORE_LIBRARY_EXPORT IconButton : public Widget
{
    Q_OBJECT

public:
    explicit IconButton(QWidget* _parent = nullptr);
    ~IconButton() override;

    /**
     * @brief Задать необходимость кнопки работать, как переключатель
     */
    void setCheckable(bool _checkable);

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
     * @brief Задать кастомный шрифт
     */
    void setCustomFont(const QFont& _font);

    /**
     * @brief Имитировать клик пользователя на кнопке для испускания сигнала
     */
    void click();

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользоатель нажал на кнопку
     */
    void clicked();

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
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
