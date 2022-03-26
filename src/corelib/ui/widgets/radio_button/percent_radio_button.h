#pragma once

#include "radio_button.h"


/**
 * @brief Радиокнопка отображающая законченность перевода
 */
class CORE_LIBRARY_EXPORT PercentRadioButton : public RadioButton
{
public:
    PercentRadioButton(QWidget* _parent, int _percents);

    /**
     * @brief Процент завершённости перевода
     */
    const int percents;

protected:
    void paintBox(QPainter& _painter, const QRectF& _rect, const QColor& _penColor) override;
};
