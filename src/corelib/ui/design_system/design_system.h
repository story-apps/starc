#pragma once

#include <QLocale>
#include <QScopedPointer>

#include <corelib_global.h>

class QColor;
class QFont;
class QMarginsF;
class QPixmap;
class QPointF;
class QSizeF;


namespace Ui {
/**
 * @brief Тема приложения
 */
enum class ApplicationTheme { Dark, Light, DarkAndLight, Custom };

/**
 * @brief Дизайн система приложения
 */
class DesignSystemPrivate;
class CORE_LIBRARY_EXPORT DesignSystem
{
public:
    /**
     * @brief Параметры цвета приложения
     * @link https://material.io/design/color/the-color-system.html
     * @link https://material.io/design/color/applying-color-to-ui.html
     * @line https://www.materialpalette.com/brown/blue
     */
    class CORE_LIBRARY_EXPORT Color
    {
    public:
        explicit Color(const Color& _rhs);
        explicit Color(const QString& _color);
        ~Color();
        Color& operator=(const Color& _rhs);

        QString toString() const;

        const QColor& primary() const;
        const QColor& secondary() const;
        const QColor& background() const;
        const QColor& surface() const;
        const QColor& error() const;
        const QColor& shadow() const;
        const QColor& onPrimary() const;
        const QColor& onSecondary() const;
        const QColor& onBackground() const;
        const QColor& onSurface() const;
        const QColor& onError() const;
        const QColor& onShadow() const;

        void setPrimary(const QColor& _color);
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
        void setOnShadow(const QColor& _color);

    private:
        Color();
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры шрифтов
     */
    class CORE_LIBRARY_EXPORT Font
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

        // Cheat sheet - https://cdn.materialdesignicons.com/5.2.45/
        const QFont& iconsSmall() const;
        const QFont& iconsMid() const;
        const QFont& iconsBig() const;
        const QFont& iconsForEditors() const;

    private:
        explicit Font(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры компоновки
     */
    class CORE_LIBRARY_EXPORT Layout
    {
    public:
        ~Layout();

        /**
         * @brief Отступ в 2 пикселя
         */
        qreal px2() const;

        /**
         * @brief Отступ в 4 пикселя
         */
        qreal px4() const;

        /**
         * @brief Отступ в 8 пикселей
         */
        qreal px8() const;

        /**
         * @brief Отступ в 12 пикселей
         */
        qreal px12() const;

        /**
         * @brief Отступ в 16 пикселей
         */
        qreal px16() const;

        /**
         * @brief Отступ в 24 пикселя
         */
        qreal px24() const;

        /**
         * @brief Отступ в 24 пикселя
         */
        qreal px48() const;

        /**
         * @brief Отступ в 62 пикселя
         */
        qreal px62() const;

        /**
         * @brief Верхний отступ контента
         */
        qreal topContentMargin() const;

        /**
         * @brief Отступы между кнопками
         */
        qreal buttonsSpacing() const;

    private:
        explicit Layout(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры панели иструментов приложения
     */
    class CORE_LIBRARY_EXPORT AppBar
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
        qreal iconsSpacing() const;

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

    private:
        explicit AppBar(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры контекстного меню
     */
    class CORE_LIBRARY_EXPORT ContextMenu
    {
    public:
        ~ContextMenu();

        /**
         * @brief Отступы контента
         */
        const QMarginsF& margins() const;

    private:
        explicit ContextMenu(qreal _scaleFactor);
        friend class DesignSystemPrivate;

        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета текстовой метки
     */
    class CORE_LIBRARY_EXPORT Label
    {
    public:
        ~Label();

        /**
         * @brief Отступы контента
         */
        const QMarginsF& margins() const;

    private:
        explicit Label(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета кнопки
     */
    class CORE_LIBRARY_EXPORT Button
    {
    public:
        ~Button();

        /**
         * @brief Высота кнопки
         */
        qreal height() const;

        /**
         * @brief Минимальная ширина
         */
        qreal minimumWidth() const;

        /**
         * @brief Отступы контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Отступ от иконки до текста
         */
        qreal spacing() const;

        /**
         * @brief Отступы вокруг кнопки, для отрисовки тени
         */
        const QMarginsF& shadowMargins() const;

        /**
         * @brief Минимальный радиус размытия тени
         */
        qreal minimumShadowBlurRadius() const;

        /**
         * @brief Максимальный радиус размытия тени
         */
        qreal maximumShadowBlurRadius() const;

        /**
         * @brief Радиус рамки кнопки
         */
        qreal borderRadius() const;

        /**
         * @brief Размер иконки
         */
        const QSizeF& iconSize() const;

    private:
        explicit Button(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета кнопки-тумблера
     */
    class CORE_LIBRARY_EXPORT ToggleButton
    {
    public:
        ~ToggleButton();

        /**
         * @brief Размер
         */
        const QSizeF& size() const;

        /**
         * @brief Отступы контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Размер иконки переключателя
         */
        const QSizeF& iconSize() const;

    private:
        explicit ToggleButton(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета переключателя
     */
    class CORE_LIBRARY_EXPORT RadioButton
    {
    public:
        ~RadioButton();

        /**
         * @brief Высота переключателя
         */
        qreal height() const;

        /**
         * @brief Отступы контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Размер иконки переключателя
         */
        const QSizeF& iconSize() const;

        /**
         * @brief Отступ между иконкой и текстом
         */
        qreal spacing() const;

    private:
        explicit RadioButton(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета флажка
     */
    class CORE_LIBRARY_EXPORT CheckBox
    {
    public:
        ~CheckBox();

        /**
         * @brief Высота переключателя
         */
        qreal height() const;

        /**
         * @brief Отступы контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Размер иконки переключателя
         */
        const QSizeF& iconSize() const;

        /**
         * @brief Отступ между иконкой и текстом
         */
        qreal spacing() const;

    private:
        explicit CheckBox(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета слайдера
     */
    class CORE_LIBRARY_EXPORT Slider
    {
    public:
        ~Slider();

        /**
         * @brief Высота всего слайдера
         */
        qreal height() const;

        /**
         * @brief Размер пипки
         */
        qreal thumbRadius() const;

        /**
         * @brief Высота полосы по которой движется пипка
         */
        qreal trackHeight() const;

        /**
         * @brief Прозрачность незаполненной части
         */
        qreal unfilledPartOpacity() const;

    private:
        explicit Slider(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета прогрессбара
     */
    class CORE_LIBRARY_EXPORT ProgressBar
    {
    public:
        ~ProgressBar();

        /**
         * @brief Высота прямого прогрессбара
         */
        qreal linearTrackHeight() const;

        /**
         * @brief Высота круглого прогрессбара
         */
        qreal circularTrackHeight() const;

        /**
         * @brief Прозрачность незаполненной части
         */
        qreal unfilledPartOpacity() const;

    private:
        explicit ProgressBar(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета текстового поля
     */
    class CORE_LIBRARY_EXPORT TextField
    {
    public:
        ~TextField();

        /**
         * @brief Цвет фона в неактивном состоянии
         */
        qreal backgroundInactiveColorOpacity() const;

        /**
         * @brief Цвет фона в активном состоянии
         */
        qreal backgroundActiveColorOpacity() const;

        /**
         * @brief Цвет обычных элементов
         */
        qreal textColorOpacity() const;

        /**
         * @brief Отступы вокруг контента
         */
        QMarginsF contentsMargins() const;

        /**
         * @brief Отступы вокруг текста
         */
        QMarginsF margins(bool _withTitle = true) const;

        /**
         * @brief Радиус скругления верхних углов
         */
        qreal borderRadius() const;

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
         * @brief Отступ от текста до иконки
         */
        qreal spacing() const;

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
        explicit TextField(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета полосы прокрутки
     */
    class CORE_LIBRARY_EXPORT ScrollBar
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
         * @brief Минимальный размер (вышины/ширины)
         */
        qreal minimumSize() const;

        /**
         * @brief Максимальный размер (вышины/ширины)
         */
        qreal maximumSize() const;

        /**
         * @brief Минимальная длина скроллера
         */
        qreal minimumHandleLength() const;

    private:
        explicit ScrollBar(qreal _scaleFactor, const Color& _color);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры панели иструментов приложения
     */
    class CORE_LIBRARY_EXPORT FloatingToolBar
    {
    public:
        ~FloatingToolBar();

        /**
         * @brief Отступы
         */
        const QMarginsF& margins() const;

        /**
         * @brief Отступы тени
         */
        const QMarginsF& shadowMargins() const;

        /**
         * @brief Минимальный радиус размытия тени
         */
        qreal minimumShadowBlurRadius() const;

        /**
         * @brief Максимальный радиус размытия тени
         */
        qreal maximumShadowBlurRadius() const;

        /**
         * @brief Высота в обычном варианте
         */
        qreal height() const;

        /**
         * @brief Размер иконки
         */
        const QSizeF& iconSize() const;

        /**
         * @brief Отступ между иконками
         */
        qreal spacing() const;

    private:
        explicit FloatingToolBar(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры вкладки
     */
    class CORE_LIBRARY_EXPORT Tab
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
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };


    /**
     * @brief Параметры виджета вкладок
     */
    class CORE_LIBRARY_EXPORT TabBar
    {
    public:
        ~TabBar();

        /**
         * @brief Отступ слева для случая, когда табы могут быть прокручены
         */
        qreal scrollableLeftMargin() const;

        /**
         * @brief Высота полоски под активным табом (входит в нижний отступ)
         */
        qreal underlineHeight() const;

    private:
        explicit TabBar(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета пошагового переключения
     */
    class CORE_LIBRARY_EXPORT Stepper
    {
    public:
        ~Stepper();

        /**
         * @brief Высота шага
         */
        qreal height() const;

        /**
         * @brief Отступы контента
         */
        QMarginsF margins() const;

        /**
         * @brief Размер иконки
         */
        QSizeF iconSize() const;

        /**
         * @brief Отступ между иконкой и текстом
         */
        qreal spacing() const;

        /**
         * @brief Отступ между иконкой и путём соединяющим несколько шагов
         */
        qreal pathSpacing() const;

        /**
         * @brief Ширина пути, соединяющиего несколько шагов
         */
        qreal pathWidth() const;

    private:
        explicit Stepper(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета выезжающего меню
     */
    class CORE_LIBRARY_EXPORT Drawer
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
        qreal spacing() const;

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

    private:
        explicit Drawer(qreal _scaleFactor, const Color& _color);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры однострочного элемента в списке
     */
    class CORE_LIBRARY_EXPORT TreeOneLineItem
    {
    public:
        ~TreeOneLineItem();

        /**
         * @brief Отступы вокруг элемента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Высота элемента без основной иконки
         */
        qreal height() const;

        /**
         * @brief Отступ между иконкой и текстом
         */
        qreal spacing() const;

        /**
         * @brief Размер иконки
         */
        const QSizeF& iconSize() const;
        const QSizeF& avatarSize() const;

    private:
        explicit TreeOneLineItem(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета списка (используем дерево как базу)
     */
    class CORE_LIBRARY_EXPORT Tree
    {
    public:
        ~Tree();

        /**
         * @brief Отступы до контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Ширина индикатора раскрывающего элементы
         */
        qreal indicatorWidth() const;

        /**
         * @brief Высота стрелочки
         */
        qreal arrowHeight() const;

        /**
         * @brief Половина ширины стрелочки
         */
        qreal arrowHalfWidth() const;

        /**
         * @brief Цвет выделения
         */
        QColor selectionColor() const;

    private:
        explicit Tree(qreal _scaleFactor, const Color& _color);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета карточки
     */
    class CORE_LIBRARY_EXPORT Card
    {
    public:
        ~Card();

        /**
         * @brief Радиус скругления рамки карточки
         */
        qreal borderRadius() const;

        /**
         * @brief Отступы тени
         */
        const QMarginsF& shadowMargins() const;

        /**
         * @brief Минимальный радиус размытия тени
         */
        qreal minimumShadowBlurRadius() const;

        /**
         * @brief Максимальный радиус размытия тени
         */
        qreal maximumShadowBlurRadius() const;

    private:
        explicit Card(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры виджета диалога
     */
    class CORE_LIBRARY_EXPORT Dialog
    {
    public:
        ~Dialog();

        /**
         * @brief Отступы до контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Минимальная ширина диалога
         */
        qreal minimumWidth() const;

        /**
         * @brief Максимальная ширина информационного диалога
         */
        qreal infoMaximumWidth() const;

    private:
        explicit Dialog(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры панели фоновых процессов
     */
    class CORE_LIBRARY_EXPORT TaskBar
    {
    public:
        ~TaskBar();

        /**
         * @brief Отступы от контента
         */
        const QMarginsF& margins() const;

        /**
         * @brief Ширина панели
         */
        qreal width() const;

        /**
         * @brief Высота одного процесса
         */
        qreal taskHeight() const;

        /**
         * @brief Высота заголовка
         */
        qreal taskTitleHeight() const;

        /**
         * @brief Отступ между заголовком и полосой прогресса
         */
        qreal spacing() const;

    private:
        explicit TaskBar(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры карточки проекта
     */
    class CORE_LIBRARY_EXPORT ProjectCard
    {
    public:
        ~ProjectCard();

        /**
         * @brief Получить размер карточки
         */
        const QSizeF& size() const;

        /**
         * @brief Отступы по краям
         */
        const QMarginsF& margins() const;

        /**
         * @brief Расстояние между карточками
         */
        qreal spacing() const;

    private:
        explicit ProjectCard(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

    /**
     * @brief Параметры карточки персонажа
     */
    class CORE_LIBRARY_EXPORT CharacterCard
    {
    public:
        ~CharacterCard();

        /**
         * @brief Получить размер карточки
         */
        const QSizeF& primarySize() const;
        const QSizeF& secondarySize() const;
        const QSizeF& tertiarySize() const;
        const QSizeF& undefinedSize() const;

        /**
         * @brief Расстояние между аватаром и текстом
         */
        qreal spacing() const;

    private:
        explicit CharacterCard(qreal _scaleFactor);
        friend class DesignSystemPrivate;
        //
        class Implementation;
        QScopedPointer<Implementation> d;
    };

public:
    /**
     * @brief Текущая тема
     */
    static ApplicationTheme theme();

    /**
     * @brief Выбрать шрифт в зависимости от языка приложения
     */
    static void updateLanguage();

    /**
     * @brief Задать тему приложения
     */
    static void setTheme(ApplicationTheme _theme);

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
     * @brief Прозрачность неактивного элемента
     */
    static qreal inactiveItemOpacity();

    /**
     * @brief Прозрачность недоступного текста
     */
    static qreal disabledTextOpacity();

    /**
     * @brief Прозрачность фона элемента под курсором
     */
    static qreal hoverBackgroundOpacity();

    /**
     * @brief Прозрачность фона сфокусированного элемента
     */
    static qreal focusBackgroundOpacity();

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
     * @brief Параметры шрифтов
     */
    static const Font& font();

    /**
     * @brief Параметры компоновки
     */
    static const Layout& layout();

    /**
     * @brief Параметры панели инструментов приложения
     */
    static const AppBar& appBar();

    /**
     * @brief Параметры текстовой метки
     */
    static const Label& label();

    /**
     * @brief Параметры контестного меню
     */
    static const ContextMenu& contextMenu();

    /**
     * @brief Параметры кнопки
     */
    static const Button& button();

    /**
     * @brief Параметры кнопки-тумблера
     */
    static const ToggleButton& toggleButton();

    /**
     * @brief Параметры кнопки-переключателя
     */
    static const RadioButton& radioButton();

    /**
     * @brief Параметры флажка
     */
    static const CheckBox& checkBox();

    /**
     * @brief Параметры слайдера
     */
    static const Slider& slider();

    /**
     * @brief Параметры прогрессбара
     */
    static const ProgressBar& progressBar();

    /**
     * @brief Параметры виджета текстового поля
     */
    static const TextField& textField();

    /**
     * @brief Параметры виджета полосы прокрутки
     */
    static const ScrollBar& scrollBar();

    /**
     * @brief Параметры плавающей панели инструментов
     */
    static const FloatingToolBar& floatingToolBar();

    /**
     * @brief Параметры виджета вкладки
     */
    static const Tab& tab();

    /**
     * @brief Параметры виджета вкладок
     */
    static const TabBar& tabBar();

    /**
     * @brief Параметры виджета пошагового движения
     */
    static const Stepper& stepper();

    /**
     * @brief Параметры выезжающего меню
     */
    static const Drawer& drawer();

    /**
     * @brief Параметры однострочного элемента списка
     */
    static const TreeOneLineItem& treeOneLineItem();

    /**
     * @brief Параметры виджета списка
     */
    static const Tree& tree();

    /**
     * @brief Параметры виджета карточки
     */
    static const Card& card();

    /**
     * @brief Параметры виджета диалога
     */
    static const Dialog& dialog();

    /**
     * @brief Параметры виджета списка фоновых процессов
     */
    static const TaskBar& taskBar();

    /**
     * @brief Параметры карточки проекта
     */
    static const ProjectCard& projectCard();

    /**
     * @brief Параметры карточки персонажа
     */
    static const CharacterCard& characterCard();

public:
    ~DesignSystem();

private:
    DesignSystem();
    QScopedPointer<DesignSystemPrivate> d;
    static DesignSystem* instance();
};

} // namespace Ui
