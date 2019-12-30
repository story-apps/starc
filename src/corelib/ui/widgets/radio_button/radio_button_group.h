#pragma once

#include <QObject>


class RadioButton;

/**
 * @brief Группа переключателей, среди которых выбран может быть только один
 */
class RadioButtonGroup : public QObject
{
    Q_OBJECT

public:
    explicit RadioButtonGroup(QObject* _parent = nullptr);
    ~RadioButtonGroup() override;

    /**
     * @brief Добавить переключатель в группу
     */
    void add(RadioButton* _radioButton);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
