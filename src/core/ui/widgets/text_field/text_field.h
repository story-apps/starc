#pragma once

#include <QTextEdit>


/**
 * @brief Виджет текстового поля
 */
class TextField : public QTextEdit
{
    Q_OBJECT

public:
    explicit TextField(QWidget* _parent = nullptr);
    ~TextField() override;

    /**
     * @brief Установить тект поясняющей метки
     */
    void setLabel(const QString& _text);

    /**
     * @brief Установить текст впомогательной подсказки
     */
    void setHelper(const QString& _text);

    /**
     * @brief Установить текст ошибки
     */
    void setError(const QString& _text);

    /**
     * @brief Получить текст поля ввода
     */
    QString text() const;

    /**
     * @brief Перекрываем собственной реализацией
     */
    void setText(const QString& _text);

    /**
     * @brief Задать иконку действия в редакторе
     */
    void setTrailingIcon(const QString& _icon);

    /**
     * @brief Включить/отключить режим ввода пароля
     */
    void setPasswordModeEnabled(bool _enable);
    bool isPasswordModeEnabled() const;

    /**
     * @brief Установить необходимость добавления новых строк при нажатии Enter'а
     */
    void setEnterMakesNewLine(bool _make);

    /**
     * @brief Перекрываем реализацию очистки своей, чтобы не ломался документ
     */
    void clear();

    /**
     * @brief Вычисляем идеальный размер в зависимости от контента
     */
    /** @{ */
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    int heightForWidth(int _width) const override;
    /** @} */

signals:
    /**
     * @brief Была нажата иконка вспомогательного действия
     */
    void trailingIconPressed();

protected:
    /**
     * @brief Задать цвет иконки вспомогательного действия
     */
    void setTrailingIconColor(const QColor& _color);

    /**
     * @brief Перенастраиваем виджет при обновлении дизайн системы
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Собственная реализация рисования
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Корректируем размер полосы декорации, при необходимости
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Переопределяем для запуска анимании декорирования
     */
    /** @{ */
    void focusInEvent(QFocusEvent* _event) override;
    void focusOutEvent(QFocusEvent* _event) override;
    /** @} */

    /**
     * @brief Переопределяем для обработки нажатия на иконке вспомогательного действия
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Корректируем курсор при наведении на иконку
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для ручной обработки некоторых клавиш
     */
    void keyPressEvent(QKeyEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

