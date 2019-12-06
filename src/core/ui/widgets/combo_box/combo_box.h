#pragma once

#include <ui/widgets/text_field/text_field.h>

class QAbstractItemModel;


/**
 * @brief Виджет выпадающего списка
 */
class ComboBox : public TextField
{
    Q_OBJECT

public:
    explicit ComboBox(QWidget* _parent = nullptr);
    ~ComboBox() override;

    void setModel(QAbstractItemModel* _model);

protected:
    /**
     * @brief Перенастраиваем виджет при обновлении дизайн системы
     */
    bool event(QEvent* _event) override;

    void mousePressEvent(QMouseEvent* _event) override;

    void focusOutEvent(QFocusEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
