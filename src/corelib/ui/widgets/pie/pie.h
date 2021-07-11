#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;

/**
 * @brief Рисует график в виде пирога или бублика
 */
class CORE_LIBRARY_EXPORT Pie : public Widget
{

    Q_OBJECT

public:
    explicit Pie(QWidget* _parent = nullptr, qreal _hole = 0);
    ~Pie() override;

    /**
     * @brief Установить модель
     */
    void setModel(const QAbstractItemModel* _model, int _valueColumn);

    /**
     * @brief Вырезать отверстие в пироге
     *
     * Значение указывается от 0 до 1.
     * Означает соотношение диаметра
     * выреза на диаметр пирога.
     */
    void setHole(qreal _hole);

signals:
    /**
     * @brief Навели машкой на кусочек пирога
     */
    void itemSelected(const QModelIndex& _index) const;

protected:
    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для изменения размера пирога
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Переопределяем для выделения кусочка пирога
     */
    void mouseMoveEvent(QMouseEvent* _event) override;


private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
