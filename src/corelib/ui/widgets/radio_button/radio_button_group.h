#pragma once

#include <corelib_global.h>

#include <QObject>


class RadioButton;

/**
 * @brief Группа переключателей, среди которых выбран может быть только один
 */
class CORE_LIBRARY_EXPORT RadioButtonGroup : public QObject
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
