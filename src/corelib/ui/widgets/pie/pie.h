#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;

class CORE_LIBRARY_EXPORT Pie : public Widget
{

    Q_OBJECT

public:
    explicit Pie(QWidget* _parent = nullptr);
    explicit Pie(const QAbstractItemModel* _model, const int _valueColumn,
                 QWidget* _parent = nullptr);
    ~Pie() override;

    /**
     * @brief Установить модель
     */
    void setModel(const QAbstractItemModel* _model, const int _valueColumn);

    /**
     * @brief Вырезать отверстие в пироге
     */
    void setHole(const double _hole);

signals:
    /**
     * @brief Навели машкой на кусочек пирога
     */
    void itemSelected(const QModelIndex& _index);

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
    /**
     * @brief Обновить размеры кусочков
     */
    void recalculateSlices();

    /**
     * @brief Ищем выбранный кусочек
     */
    void updateSelectedSlice();

    /**
     * @brief Подключаемся к сигналам модели
     */
    void connectSignals(const QAbstractItemModel* _model);

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
     * @brief В моделе изменились данные
     */
    void changeData(const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                    const QVector<int>& _roles);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
