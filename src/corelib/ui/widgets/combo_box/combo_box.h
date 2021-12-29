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
     * @brief Необходимо ли расширят размер попапа, чтобы содержимое полностью вмещалось
     */
    void setUseContentsWidth(bool _use);

    /**
     * @brief Отключаем контекстное меню для комбобокса
     */
    ContextMenu* createContextMenu(const QPoint& _position, QWidget* _parent = nullptr) override;

    /**
     * @brief Модель выпадающего списка
     */
    QAbstractItemModel* model() const;
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Индекс текущего выбранного элемента
     */
    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Текст текущего выбранного элемента
     */
    QString currentText() const;
    void setCurrentText(const QString& _text);

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
     * @brief Добавляем обработку кнопок винз/вверх
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Реализуем отображение/скрытие попапа
     */
    void mousePressEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
