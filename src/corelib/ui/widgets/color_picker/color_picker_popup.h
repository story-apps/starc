#pragma once

#include <ui/widgets/card/card.h>


/**
 * @brief Виджет всплывающего окошка для выбора цвета
 */
class CORE_LIBRARY_EXPORT ColorPickerPopup : public Card
{
    Q_OBJECT

public:
    explicit ColorPickerPopup(QWidget* _parent = nullptr);
    ~ColorPickerPopup() override;

    /**
     * @brief Выбранный пользователем цвет
     */
    QColor selectedColor() const;

    /**
     * @brief Задать выбранный пользователем цвет
     */
    void setSelectedColor(const QColor& _color);

    /**
     * @brief Показан ли попап в данный момент
     */
    bool isPopupShown() const;

    /**
     * @brief Показать попап выбора цвета
     */
    void showPopup(QWidget* _parent);

    /**
     * @brief Скрыть попап выбора цвета
     */
    void hidePopup();

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void selectedColorChanged(const QColor& _color);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
