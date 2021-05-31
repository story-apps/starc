#pragma once

#include <ui/widgets/widget/widget.h>

class CORE_LIBRARY_EXPORT Pie : public Widget {
public:
    explicit Pie(QWidget* _parent = nullptr);
    ~Pie() override;

    void addItem(const double _value, const QColor& _color);
    void setHole(const double _hole);

signals:
    void itemSelected();

protected:
    /**
   * @brief Реализуем собственную отрисовку
   */
    void paintEvent(QPaintEvent* _event) override;
    void resizeEvent(QResizeEvent* _event) override;

    void mouseMoveEvent(QMouseEvent* _event) override;

private:
    void recalculateSlices();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
