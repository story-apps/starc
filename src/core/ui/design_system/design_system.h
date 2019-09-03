#pragma once

#include <QScopedPointer>

class QColor;
class QFont;
class QMarginsF;
class QPixmap;
class QPointF;
class QSizeF;


namespace Ui
{
/**
 * @brief Дизайн система приложения
 */
class DesignSystemPrivate;
class DesignSystem
{
public:
    /**
     * @brief Параметры цвета приложения
     * @link https://material.io/design/color/the-color-system.html
     * @link https://material.io/design/color/applying-color-to-ui.html
     * @line https://www.materialpalette.com/brown/blue
     */
    class ColorPrivate;
    class Color
    {
    public:
        explicit Color(const Color& _rhs);
        ~Color();
        Color& operator=(const Color& _rhs);

        QColor primary() const;
        QColor primaryDark() const;
        QColor secondary() const;
        QColor background() const;
        QColor surface() const;
        QColor error() const;
        QColor shadow() const;
        QColor onPrimary() const;
        QColor onSecondary() const;
        QColor onBackground() const;
        QColor onSurface() const;
        QColor onError() const;

        void setPrimary(const QColor& _color);
        void setPrimaryDark(const QColor& _color);
        void setSecondary(const QColor& _color);
        void setBackground(const QColor& _color);
        void setSurface(const QColor& _color);
        void setError(const QColor& _color);
        void setShadow(const QColor& _color);
        void setOnPrimary(const QColor& _color);
        void setOnSecondary(const QColor& _color);
        void setOnBackground(const QColor& _color);
        void setOnSurface(const QColor& _color);
        void setOnError(const QColor& _color);

    private:
        Color();
        QScopedPointer<ColorPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры панели иструментов приложения
     */
    class AppBarPrivate;
    class AppBar
    {
    public:
        ~AppBar();

        /**
         * @brief Отступы
         */
        QMarginsF margins() const;

        /**
         * @brief Отступы для большой иконки
         */
        QMarginsF bigIconMargins() const;

        /**
         * @brief Высота в обычном варианте
         */
        qreal heightRegular() const;

        /**
         * @brief Размер иконки
         */
        QSizeF iconSize() const;

        /**
         * @brief Размер большой иконки (которая расположена перед текстом)
         */
        QSizeF bigIconSize() const;

        /**
         * @brief Отступ между иконками
         */
        qreal iconsMargin() const;

        /**
         * @brief Левый отступ текста
         */
        qreal leftTitleMargin() const;

        /**
         * @brief Радиус размытия тени
         */
        qreal shadowRadius() const;

        /**
         * @brief Отступ для рисования тени
         */
        QPointF shadowOffset() const;

        /**
         * @brief Шрифт
         */
        QFont font() const;

    private:
        explicit AppBar(qreal _scaleFactor);
        QScopedPointer<AppBarPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета выезжающего меню
     */
    class DrawerPrivate;
    class Drawer
    {
    public:
        ~Drawer();

        /**
         * @brief Отступы
         */
        QMarginsF margins() const;

        /**
         * @brief Отступы пункта меню
         */
        QMarginsF actionMargins() const;

        /**
         * @brief Отступы выделения
         */
        QMarginsF selectionMargins() const;

        /**
         * @brief Отступ от подзаголовка до первого пункта меню
         */
        qreal subtitleBottomMargin() const;

        /**
         * @brief Отступ от иконки до текста пункта меню
         */
        qreal iconRightMargin() const;

        /**
         * @brief Ширина
         */
        qreal width() const;

        /**
         * @brief Высота заголовка
         */
        qreal titleHeight() const;

        /**
         * @brief Высота подзаголовка
         */
        qreal subtitleHeight() const;

        /**
         * @brief Высота пункта меню
         */
        qreal actionHeight() const;

        /**
         * @brief Размер иконки
         */
        QSizeF iconSize() const;

        /**
         * @brief Высота разделительной полосы
         */
        qreal separatorHeight() const;

        /**
         * @brief Отступы до разделительной полосы
         */
        qreal separatorSpacing() const;

        /**
         * @brief Цвет выделения
         */
        QColor selectionColor() const;

        /**
         * @brief Шрифт заголовка
         */
        QFont titleFont() const;

        /**
         * @brief Шрифт подзаголовка
         */
        QFont subtitleFont() const;

        /**
         * @brief Шрифт пунктов меню
         */
        QFont actionFont() const;

    private:
        explicit Drawer(qreal _scaleFactor, const Color& _color);
        QScopedPointer<DrawerPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета таба
     */
    class TabPrivate;
    class Tab
    {
    public:
        ~Tab();

        /**
         * @brief Минимальная ширина
         */
        qreal minimumWidth() const;

        /**
         * @brief Высота
         */
        /** @{ */
        qreal heightWithText() const;
        qreal heightWithIcon() const;
        qreal heightWithTextAndIcon() const;
        /** @} */

        /**
         * @brief Отступы вокруг
         */
        QMarginsF margins() const;

        /**
         * @brief Размер иконки
         */
        QSizeF iconSize() const;

    private:
        explicit Tab(qreal _scaleFactor);
        QScopedPointer<TabPrivate> d;
        friend class DesignSystemPrivate;
    };

    // ****

    /**
     * @brief Параметры виджета вкладок
     */
    class TabsPrivate;
    class Tabs
    {
    public:
        ~Tabs();

        /**
         * @brief Отступ слева для случая, когда табы могут быть прокручены
         */
        qreal scrollableLeftMargin() const;

        /**
         * @brief Высота полоски под активным табом (входит в нижний отступ)
         */
        qreal underlineHeight() const;

        /**
         * @brief Шрифт
         */
        QFont font() const;

    private:
        explicit Tabs(qreal _scaleFactor);
        QScopedPointer<TabsPrivate> d;
        friend class DesignSystemPrivate;
    };

    // ****

    /**
     * @brief Параметры виджета текстового поля
     */
    class TextFieldPrivate;
    class TextField
    {
    public:
        ~TextField();

        /**
         * @brief Цвет фона в неактивном состоянии
         */
        QColor backgroundInactiveColor() const;

        /**
         * @brief Цвет фона в активном состоянии
         */
        QColor backgroundActiveColor() const;

        /**
         * @brief Цвет обычных элементов
         */
        QColor foregroundColor() const;

        /**
         * @brief Шрифт для лейбла
         */
        QFont labelFont() const;

        /**
         * @brief Шрифт для текста
         */
        QFont textFont() const;

        /**
         * @brief Шрифт для вспомогательного текста
         */
        QFont helperFont() const;

        /**
         * @brief Отступы вокруг
         */
        QMarginsF margins() const;

        /**
         * @brief Координата отрисовки лейбла
         */
        QPointF labelTopLeft() const;

        /**
         * @brief Координата верха иконки
         */
        qreal iconTop() const;

        /**
         * @brief Размер иконки
         */
        QSizeF iconSize() const;

        /**
         * @brief Высота полоски под редактором не в фокусе
         */
        qreal underlineHeight() const;

        /**
         * @brief Высота полоски под редактором в фокусе
         */
        qreal underlineHeightInFocus() const;

        /**
         * @brief Высота вспомогательного текста под редактором
         */
        qreal helperHeight() const;

    private:
        explicit TextField(qreal _scaleFactor, const Color& _color);
        QScopedPointer<TextFieldPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета выбора цвета
     */
    class ColorPickerPrivate;
    class ColorPicker
    {
    public:
        ~ColorPicker();

        /**
         * @brief Отступы
         */
        QMarginsF margins() const;

        /**
         * @brief Высота виджета
         */
        qreal height() const;

        /**
         * @brief Размер иконки с цветом
         */
        QSizeF iconSize() const;

        /**
         * @brief Отступ между соседними иконками
         */
        qreal iconsSpacing() const;

        /**
         * @brief Ширина линии обводки иконки
         */
        qreal iconBorderWidth() const;

    private:
        explicit ColorPicker(qreal _scaleFactor);
        QScopedPointer<ColorPickerPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета текстового переключателя
     */
    class TextTogglePrivate;
    class TextToggle
    {
    public:
        ~TextToggle();

        /**
         * @brief Отступы вокруг
         */
        QMarginsF margins() const;

        /**
         * @brief Размер всего переключателя
         */
        QSizeF toggleSize() const;

        /**
         * @brief Радиус скругления переключателя
         */
        qreal toggleRadius() const;

        /**
         * @brief Отступ от текста до переключателя
         */
        qreal spacing() const;

        /**
         * @brief Отступ от края переключателя до рамки
         */
        qreal toggleSpacing() const;

        /**
         * @brief Шрифт текста
         */
        QFont font() const;

    private:
        explicit TextToggle(qreal _scaleFactor);
        QScopedPointer<TextTogglePrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета полосы прокрутки
     */
    class ScrollBarPrivate;
    class ScrollBar
    {
    public:
        ~ScrollBar();

        /**
         * @brief Цвет фона
         */
        QColor backgroundColor() const;

        /**
         * @brief Цвет хэндла
         */
        QColor handleColor() const;

        /**
         * @brief Отступы
         */
        QMarginsF margins() const;

        /**
         * @brief Ширина вертикального скролбара
         */
        qreal verticalWidth() const;

        /**
         * @brief Высота горизонтального скролбара
         */
        qreal horizontalHeight() const;

    private:
        explicit ScrollBar(qreal _scaleFactor, const Color& _color);
        QScopedPointer<ScrollBarPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры двустрочного элемента в списке
     */
    class ListTwoLineItemPrivate;
    class ListTwoLineItem
    {
    public:
        ~ListTwoLineItem();

        /**
         * @brief Отступы вокруг элемента
         */
        QMarginsF margins() const;

        /**
         * @brief Верхний отступ для вспомогательной иконки
         */
        qreal secondaryIconTopMargin() const;

        /**
         * @brief Отступы между иконками
         */
        qreal iconsSpacing() const;

        /**
         * @brief Высота элемента без основной иконки
         */
        qreal height() const;

        /**
         * @brief Высота элемента, когда есть основная иконка
         */
        qreal heightWithMainIcon() const;

        /**
         * @brief Высота тени
         */
        qreal shadowHeight() const;

        /**
         * @brief Размер основной иконки
         */
        QSizeF mainIconSize() const;

        /**
         * @brief Размер дополнительной иконки
         */
        QSizeF secondaryIconSize() const;

        /**
         * @brief Цвет текущего элемента
         */
        QColor currentItemColor() const;

        /**
         * @brief Шрифт первой строки текста
         */
        QFont firstLineFont() const;

        /**
         * @brief Шрифт второй строки текста
         */
        QFont secondLineFont() const;

    private:
        explicit ListTwoLineItem(qreal _scaleFactor, const Color& _colors);
        QScopedPointer<ListTwoLineItemPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета списка
     */
    class ListPrivate;
    class List
    {
    public:
        ~List();

        /**
         * @brief Отступы до контента
         */
        QMarginsF margins() const;

    private:
        explicit List(qreal _scaleFactor);
        QScopedPointer<ListPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры виджета диалога
     */
    class DialogPrivate;
    class Dialog
    {
    public:
        ~Dialog();

        /**
         * @brief Отступы до контента
         */
        QMarginsF margins() const;

        /**
         * @brief Отступы до кнопок
         */
        QMarginsF buttonsMargins() const;

        /**
         * @brief Ширина диалога
         */
        qreal width() const;

        /**
         * @brief Отступ между заголовком и описанием
         */
        qreal textSpacing() const;

        /**
         * @brief Высота области, в которой располагаются кнопки
         */
        qreal buttonsHeight() const;

        /**
         * @brief Расстояние между кнопками
         */
        qreal buttonsSpacing() const;

        /**
         * @brief Радиус тени
         */
        qreal shadowRadius() const;

        /**
         * @brief Шрифт заголовка
         */
        QFont titleFont() const;

        /**
         * @brief Шрифт поясняющего текста
         */
        QFont supportingTextFont() const;

        /**
         * @brief Шрифт кнопок
         */
        QFont buttonsFont() const;

    private:
        explicit Dialog(qreal _scaleFactor);
        QScopedPointer<DialogPrivate> d;
        friend class DesignSystemPrivate;
    };

    /**
     * @brief Параметры шрифтов
     */
    class Font
    {
    public:
        ~Font();

        const QFont& h1() const;
        const QFont& h2() const;
        const QFont& h3() const;
        const QFont& h4() const;
        const QFont& h5() const;
        const QFont& h6() const;
        const QFont& subtitle1() const;
        const QFont& subtitle2() const;
        const QFont& body1() const;
        const QFont& body2() const;
        const QFont& button() const;
        const QFont& caption() const;
        const QFont& overline() const;

        // Cheat sheet - https://cdn.materialdesignicons.com/3.8.95/
        const QFont& iconsSmall() const;

    private:
        explicit Font(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета пошагового переключения
     */
    class Stepper
    {
    public:
        ~Stepper();

        qreal stepHeight() const;

        QMarginsF margins() const;

        qreal textSpacing() const;

        qreal pathSpacing() const;

        QSizeF iconSize() const;

        qreal pathWidth() const;

    private:
        explicit Stepper(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

public:
    /**
     * @brief Текущий индекс масштабирования
     */
    static qreal scaleFactor();

    /**
     * @brief Задать коэффициент масштабирования приложения
     */
    static void setScaleFactor(qreal _scaleFactor);

    /**
     * @brief Отступы у страниц
     */
    static QMarginsF pageMargins();

    /**
     * @brief Отступы между элементами на страницах
     */
    static qreal pageSpacing();

    /**
     * @brief Прозрачность неактивного текста
     */
    static qreal inactiveTextOpacity();

    /**
     * @brief Прозрачность недоступного текста
     */
    static qreal disabledTextOpacity();

    /**
     * @brief Прозрачность залития эффекта выделения
     */
    /** @{ */
    static qreal elevationStartOpacity();
    static qreal elevationEndOpacity();
    /** @} */

    /**
     * @brief Параметры цвета приложения
     */
    static const Color& color();
    static void setColor(const Color& _color);

    /**
     * @brief Параметры панели инструментов приложения
     */
    static const AppBar& appBar();

    /**
     * @brief Параметры выезжающего меню
     */
    static const Drawer& drawer();

    /**
     * @brief Параметры виджета вкладки
     */
    static const Tab& tab();

    /**
     * @brief Параметры виджета вкладок
     */
    static const Tabs& tabs();

    /**
     * @brief Параметры виджета текстового поля
     */
    static const TextField& textField();

    /**
     * @brief Параметры виджета выбора цвета
     */
    static const ColorPicker& colorPicker();

    /**
     * @brief Параметры виджета переключателя с текстом
     */
    static const TextToggle& textToggle();

    /**
     * @brief Параметры виджета полосы прокрутки
     */
    static const ScrollBar& scrollBar();

    /**
     * @brief Параметры двухстрочного элемента списка
     */
    static const ListTwoLineItem& listTwoLineItem();

    /**
     * @brief Параметры виджета списка
     */
    static const List& list();

    /**
     * @brief Параметры виджета диалога
     */
    static const Dialog& dialog();





    /**
     * @brief Параметры шрифтов
     */
    static const Font& font();

    /**
     * @brief Параметры виджета пошагового движения
     */
    static const Stepper& stepper();

public:
    ~DesignSystem();

private:
    DesignSystem();
    QScopedPointer<DesignSystemPrivate> d;
    static DesignSystem* instance();
};

} // namespace Ui
