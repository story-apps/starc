#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет панели инструментов приложения
 */
class AppBar : public Widget
{
    Q_OBJECT

public:
    explicit AppBar(QWidget* _parent = nullptr);
    ~AppBar() override;

signals:

protected:
    /**
     * @brief Реализуем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

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
