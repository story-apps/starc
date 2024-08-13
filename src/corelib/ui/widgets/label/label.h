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
    virtual void setText(const QString& _text);

    /**
     * @brief Скругление углов
     */
    qreal borderRadius() const;
    void setBorderRadius(qreal _radius);

    /**
     * @brief Задать текст для отрисовки области заливки
     */
    void setSkeleton(const QString& _filler);

    /**
     * @brief Сделать шрифт зачёркнутым
     */
    void setStrikeOut(bool _strikeOut);

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
    QFont textFont() const;
    virtual const QFont& textFontImpl() const = 0;

    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для открытия ссылки при клике на виджет
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    /**
     * @brief Перекрываем метод, чтобы клиенты его не использовали
     */
    void setFont(const QFont& _font);

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
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка заголовка пятого уровня
 */
class CORE_LIBRARY_EXPORT H5Label : public AbstractLabel
{
public:
    explicit H5Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка заголовка шестого уровня
 */
class CORE_LIBRARY_EXPORT H6Label : public AbstractLabel
{
public:
    explicit H6Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом subtitle1
 */
class CORE_LIBRARY_EXPORT Subtitle1Label : public AbstractLabel
{
public:
    explicit Subtitle1Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом subtitle2
 */
class CORE_LIBRARY_EXPORT Subtitle2Label : public AbstractLabel
{
public:
    explicit Subtitle2Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом body1
 */
class CORE_LIBRARY_EXPORT Body1Label : public AbstractLabel
{
public:
    explicit Body1Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом body2
 */
class CORE_LIBRARY_EXPORT Body2Label : public AbstractLabel
{
public:
    explicit Body2Label(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом button
 */
class CORE_LIBRARY_EXPORT ButtonLabel : public AbstractLabel
{
public:
    explicit ButtonLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом caption
 */
class CORE_LIBRARY_EXPORT CaptionLabel : public AbstractLabel
{
public:
    explicit CaptionLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом overline
 */
class CORE_LIBRARY_EXPORT OverlineLabel : public AbstractLabel
{
public:
    explicit OverlineLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};

class CORE_LIBRARY_EXPORT AbstractIconsLabel : public AbstractLabel
{
public:
    explicit AbstractIconsLabel(QWidget* _parent = nullptr);

    /**
     * @brief Установить иконку
     */
    void setIcon(const QString& _icon);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Корректируем иконку при смене направления расположения текста
     */
    bool event(QEvent* _event) override;

private:
    /**
     * @brief Закрываем метод, чтобы клиенты не могли использовать неверный способ задания иконки
     */
    void setText(const QString& _text) override;

    /**
     * @brief Исходная иконка
     */
    QString m_icon;
};


/**
 * @brief Текстовая метка со шрифтом iconSmall
 */
class CORE_LIBRARY_EXPORT IconsSmallLabel : public AbstractIconsLabel
{
public:
    explicit IconsSmallLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Текстовая метка со шрифтом iconMid
 */
class CORE_LIBRARY_EXPORT IconsMidLabel : public AbstractIconsLabel
{
public:
    explicit IconsMidLabel(QWidget* _parent = nullptr);

    /**
     * @brief Необходимо ли рисовать декорацию
     */
    void setDecorationVisible(bool _visible);

    /**
     * @brief Учитываем размер декорации при определении идеального размера
     */
    QSize sizeHint() const override;
    int heightForWidth(int _width) const override;

protected:
    /**
     * @brief Необходимый шрифт для отрисовки лейбла
     */
    const QFont& textFontImpl() const override;

    /**
     * @brief Сначала рисуем декорацию, а потом иконку
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    /**
     * @brief Необходимо ли отображать декорацию
     */
    bool m_isDecorationVisible = false;
};


/**
 * @brief Текстовая метка со шрифтом iconBig
 */
class CORE_LIBRARY_EXPORT IconsBigLabel : public AbstractIconsLabel
{
public:
    explicit IconsBigLabel(QWidget* _parent = nullptr);

    /**
     * @brief Необходимо ли рисовать декорацию
     */
    void setDecorationVisible(bool _visible);

    /**
     * @brief Учитываем размер декорации при определении идеального размера
     */
    QSize sizeHint() const override;
    int heightForWidth(int _width) const override;

protected:
    /**
     * @brief Необходимый шрифт для отрисовки лейбла
     */
    const QFont& textFontImpl() const override;

    /**
     * @brief Сначала рисуем декорацию, а потом иконку
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    /**
     * @brief Необходимо ли отображать декорацию
     */
    bool m_isDecorationVisible = false;
};

/**
 * @brief Виджет для отображения изображения
 */
class CORE_LIBRARY_EXPORT ImageLabel : public Widget
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget* _parent = nullptr);
    ~ImageLabel() override;

    /**
     * @brief Скругление углов
     */
    qreal borderRadius() const;
    void setBorderRadius(qreal _radius);

    /**
     * @brief Использовать скелетон, если не задано изображение
     */
    void setSkeleton(bool _enabled);

    /**
     * @brief Изображение для отрисовки
     */
    QPixmap image() const;
    void setImage(const QPixmap& _image);

signals:
    /**
     * @brief Пользователь кликнул на виджете
     */
    void clicked();

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Обновляем закешированную картинку, чтобы она соответствовала актуальному размеру
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Переопределяем для открытия ссылки при клике на виджет
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
