#pragma once

#include <QSplitterHandle>


/**
 * @brief Виджет разделитель сплитера
 */
class SplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    explicit SplitterHandle(Qt::Orientation _orientation, QSplitter* _parent = nullptr);
    ~SplitterHandle() override;

    /**
     * @brief Цвет фона виджета
     */
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& _color);

protected:
    /**
     * @brief Переопределяем для собственной перерисовки
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
