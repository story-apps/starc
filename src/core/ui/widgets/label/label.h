#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Базовый класс для виджетов текстовых меток
 */
class AbstractLabel : public Widget
{
public:
    explicit AbstractLabel(QWidget *_parent = nullptr);
    ~AbstractLabel() override;

    /**
     * @brief Задать текст
     */
    void setText(const QString& _text);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

protected:
    /**
     * @brief Получить шрифт для отрисовки текста
     */
    virtual const QFont& textFont() const = 0;

    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


/**
 * @brief Текстовая метка заголовка пятого уровня
 */
class H5Label : public AbstractLabel
{
public:
    explicit H5Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка заголовка шестого уровня
 */
class H6Label : public AbstractLabel
{
public:
    explicit H6Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом body1
 */
class Body1Label : public AbstractLabel
{
public:
    explicit Body1Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом body2
 */
class Body2Label : public AbstractLabel
{
public:
    explicit Body2Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};
