#pragma once

#include <ui/widgets/text_field/text_field.h>

class QAbstractItemModel;


/**
 * @brief Виджет выпадающего списка
 */
class CORE_LIBRARY_EXPORT ComboBox : public TextField
{
    Q_OBJECT

public:
    explicit ComboBox(QWidget* _parent = nullptr);
    ~ComboBox() override;

    /**
     * @brief Задать модель выпадающего списка
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Индекс текущего выбранного элемента
     */
    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex& _index);

signals:
    /**
     * @brief Изменился текущий индекс
     */
    void currentIndexChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Сконфигурируем попап
     */
    void reconfigure() override;

    /**
     * @brief Скрываем попап, когда фокус ушёл из виджета
     */
    void focusOutEvent(QFocusEvent* _event) override;

    /**
     * @brief Реализуем отображение/скрытие попапа
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы пропустить обработку связанную с курсором редактора TextField'а
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
