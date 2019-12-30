#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет реализующий пошаговый проход состояний
 */
class Stepper : public Widget
{
    Q_OBJECT

public:
    explicit Stepper(QWidget* _parent = nullptr);
    ~Stepper() override;

    /**
     * @brief Задать цвет фона номера неактивного шага
     */
    void setInactiveStepNumberBackgroundColor(const QColor& _color);

    /**
     * @brief Добавить шаг
     */
    void addStep(const QString& _stepName);

    /**
     * @brief Задать имя для заданного шага
     */
    void setStepName(int _index, const QString& _name);

    /**
     * @brief Задать индекс текущего шага
     */
    void setCurrentStep(int _index);

    /**
     * @brief Установить состояние завершённости всех шагов
     */
    void setFinished(bool _finished);

signals:
    /**
     * @brief Изменился текущий шаг
     */
    void currentIndexChanged(int _currentIndex, int _previousIndex);

protected:
    /**
     * @brief Собственная реализация рисования
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Смена активного шага при клике мышкой
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
