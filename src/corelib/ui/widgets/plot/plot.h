#pragma once

#include "qcustomplot.h"

#include <corelib_global.h>


/**
 * @brief Виджет для отображения графиков, расширяющий возможности QCustomPlot
 */
class CORE_LIBRARY_EXPORT Plot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit Plot(QWidget* _parent = 0);
    ~Plot() override;

    void setPlotInfo(const QMap<qreal, QStringList>& _info);

protected:
    /**
     * @brief
     */
    void paintEvent(QPaintEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
