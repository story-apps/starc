#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Базовый класс для виджетов текстовых меток
 */
class CORE_LIBRARY_EXPORT AbstractLabel : public Widget
{
    Q_OBJECT

public:
    explicit AbstractLabel(QWidget* _parent = nullptr);
    ~AbstractLabel() override;

    /**
     * @brief Получить текст
     */
    QString text() const;

    /**
     * @brief Задать текст
     */
    void setText(const QString& _text);

    /**
     * @brief Задать выравнивание текста
     */
    void setAlignment(Qt::Alignment _alignment);

    /**
     * @brief Возможность кликнуть на лейбле
     */
    void setClickable(bool _clickable);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;
    int heightForWidth(int _width) const override;

signals:
    /**
     * @brief Пользователь кликнул на виджете
     */
    void clicked();

protected:
    /**
     * @brief Получить шрифт для отрисовки текста
     */
    virtual const QFont& textFont() const = 0;

    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для открытия ссылки при клике на виджет
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


/**
 * @brief Текстовая метка заголовка пятого уровня
 */
class CORE_LIBRARY_EXPORT H4Label : public AbstractLabel
{
public:
    explicit H4Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка заголовка пятого уровня
 */
class CORE_LIBRARY_EXPORT H5Label : public AbstractLabel
{
public:
    explicit H5Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка заголовка шестого уровня
 */
class CORE_LIBRARY_EXPORT H6Label : public AbstractLabel
{
public:
    explicit H6Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом subtitle1
 */
class CORE_LIBRARY_EXPORT Subtitle1Label : public AbstractLabel
{
public:
    explicit Subtitle1Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом subtitle2
 */
class CORE_LIBRARY_EXPORT Subtitle2Label : public AbstractLabel
{
public:
    explicit Subtitle2Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом body1
 */
class CORE_LIBRARY_EXPORT Body1Label : public AbstractLabel
{
public:
    explicit Body1Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом body2
 */
class CORE_LIBRARY_EXPORT Body2Label : public AbstractLabel
{
public:
    explicit Body2Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом caption
 */
class CORE_LIBRARY_EXPORT CaptionLabel : public AbstractLabel
{
public:
    explicit CaptionLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом overline
 */
class CORE_LIBRARY_EXPORT OverlineLabel : public AbstractLabel
{
public:
    explicit OverlineLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом iconMid
 */
class CORE_LIBRARY_EXPORT IconsMidLabel : public AbstractLabel
{
public:
    explicit IconsMidLabel(QWidget* _parent = nullptr);

    /**
     * @brief Установить иконку
     */
    void setIcon(const QString& _icon);

protected:
    const QFont& textFont() const override;
};


/**
 * @brief Текстовая метка со шрифтом iconBig
 */
class CORE_LIBRARY_EXPORT IconsBigLabel : public AbstractLabel
{
public:
    explicit IconsBigLabel(QWidget* _parent = nullptr);

    /**
     * @brief Установить иконку
     */
    void setIcon(const QString& _icon);

protected:
    const QFont& textFont() const override;
};
