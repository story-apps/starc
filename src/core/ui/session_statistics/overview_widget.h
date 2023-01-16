#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет обзорной информации для дэшборда
 */
class OverviewWidget : public Widget
{
    Q_OBJECT

public:
    explicit OverviewWidget(QWidget* _parent = nullptr);
    ~OverviewWidget() override;

    /**
     * @brief Задать данные
     */
    void setTitle(const QString _icon, const QString& _title);
    void setColors(const QColor& _min, const QColor& _avg, const QColor& _max);
    void setLabels(const QString& _min, const QString& _avg, const QString& _max);
    void setValues(const QString& _min, const QString& _avg, const QString& _max);
    void setProgress(int _min, int _avg, int _max, const QString& _progressTitle);

    /**
     * @brief Переопределяем рассчёт идеального размера для виджета
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
