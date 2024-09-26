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
     * @brief Задать кастомную палитру цветов
     */
    void setCustomPalette(const QVector<QColor>& _palette);

    /**
     * @brief Задать возможность удаления выбранного цвета, если выбрать его ещё раз
     */
    void setColorCanBeDeselected(bool _can);

    /**
     * @brief Задать необходимость закрытия попапа после выборе цвета
     */
    void setAutoHideOnSelection(bool _autoHide);

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
    void showPopup(QWidget* _parent, Qt::Alignment _alignment = Qt::AlignBottom | Qt::AlignHCenter);

    /**
     * @brief Скрыть попап выбора цвета
     */
    void hidePopup();

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void selectedColorChanged(const QColor& _color);

protected:
    /**
     * @brief Прокидываем изменение цвета в дочерние виджеты
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
