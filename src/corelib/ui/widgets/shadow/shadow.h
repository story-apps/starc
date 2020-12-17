#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет тени, использующийся для декорирования основных виджетов
 */
class CORE_LIBRARY_EXPORT Shadow : public Widget
{
    Q_OBJECT

public:
    explicit Shadow(QWidget* _parent = nullptr);
    Shadow(Qt::Edge _edge, QWidget* _parent = nullptr);

    /**
     * @brief Задать сторону, куда прилепиться
     */
    void setEdge(Qt::Edge _edge);

protected:
    /**
     * @brief Ловим события родительского виджета, чтобы корректировать свой размер и положение
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Переопределяем для отрисовки тени
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief При смене параметров дизайн-системы нужно обновить собственную геометрию
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    /**
     * @brief Обновить геометрию
     */
    void refreshGeometry();

private:
    /**
     * @brief Сторона с которой отображается виджет
     */
    Qt::Edge m_edge = Qt::LeftEdge;
};
