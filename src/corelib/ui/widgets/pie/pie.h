#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;

class CORE_LIBRARY_EXPORT Pie : public Widget
{

    Q_OBJECT

public:
    explicit Pie(QWidget* _parent = nullptr);
    explicit Pie(const QAbstractItemModel* model, const int valueColumn,
                 QWidget* _parent = nullptr);
    ~Pie() override;

    /**
     * @brief Установить модель
     */
    void setModel(const QAbstractItemModel* model, const int valueColumn);

    /**
     * @brief Вырезать отверстие в пироге
     */
    void setHole(const double _hole);

signals:
    /**
     * @brief Навели машкой на кусочек пирога
     */
    void itemSelected(const QModelIndex& index);

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
    void connectSignals(const QAbstractItemModel* model);

private slots:
    /**
     * @brief В модель пришли новые данные
     */
    void insertSlices(const QModelIndex& parent, int first, int last);

    /**
     * @brief Из модели удалили данные
     */
    void removeSlices(const QModelIndex& parent, int first, int last);

    /**
     * @brief В моделе изменились данные
     */
    void changeData(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                    const QVector<int>& roles);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
