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
    explicit Pie(QWidget* _parent = nullptr, double _hole = 0);
    explicit Pie(const QAbstractItemModel* _model, int _valueColumn, double _hole = 0,
                 QWidget* _parent = nullptr);
    ~Pie() override;

    /**
     * @brief Установить модель
     */
    void setModel(const QAbstractItemModel* _model, const int _valueColumn);

    /**
     * @brief Вырезать отверстие в пироге
     *
     * Значение указывается от 0 до 1.
     * Означает соотношение диаметра
     * выреза на диаметр пирога.
     */
    void setHole(double _hole);

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

private slots:
    /**
     * @brief В модель пришли новые данные
     */
    void insertSlices(const QModelIndex& _parent, int _first, int _last);

    /**
     * @brief Из модели удалили данные
     */
    void removeSlices(const QModelIndex& _parent, int _first, int _last);

    /**
     * @brief В модели изменились данные
     */
    void changeData(const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                    const QVector<int>& _roles);


private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
