#pragma once

#include <QHash>
#include <QPageSize>
#include <QTextFormat>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer
{

/**
 * @brief Типы параграфов в сценарии
 */
enum class ScreenplayParagraphType {
    Undefined,
    UnformattedText,
    SceneHeading,
    SceneCharacters,
    Action,
    Character,
    Parenthetical,
    Dialogue,
    Lyrics,
    Transition,
    Shot,
    InlineNote,
    FolderHeader,
    FolderFooter,
    //
    SceneHeadingShadow,	//!< Время и место, для вспомогательных разрывов
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
inline uint qHash(ScreenplayParagraphType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить текстовое представление типа блока
 */
QString toString(ScreenplayParagraphType _type);

/**
 * @brief Получить тип блока из текстового представления
 */
ScreenplayParagraphType screenplayParagraphTypeFromString(const QString& _text);


/**
 * @brief Класс стиля блока сценария
 */
class ScreenplayBlockStyle
{
public:
    /**
     * @brief Дополнительные свойства стилей текстовых блоков
     */
    enum Property {
        PropertyType = QTextFormat::UserProperty + 100, //!< Тип блока
        PropertyHeaderType,       //!< Тип блока заголовка
        PropertyHeader,           //!< Текст заголовка блока (а-ля "ТИТР:")
        PropertyPrefix,           //!< Префикс блока
        PropertyPostfix,          //!< Постфикс блока
        PropertyIsFirstUppercase, //!< Необходимо ли первый символ поднимать в верхний регистр
        PropertyIsCanModify,      //!< Редактируемый ли блок
        //
        // Свойства редакторских заметок
        //
        PropertyIsReviewMark,    //!< Формат является редакторской правкой
        PropertyIsHighlight,     //!< Является ли правка аналогом выделения цветом из ворда
        PropertyIsDone,          //!< Правка помечена как выполненная
        PropertyComments,        //!< Список комментариев к правке
        PropertyCommentsAuthors, //!< Список авторов комментариев
        PropertyCommentsDates,   //!< Список дат комментариев
        //
        // Свойства корректирующих текст блоков
        //
        PropertyIsCorrection,           //!< Не разрывающий текст блок (пустые блоки в конце страницы, блоки с текстом ПРОД, или именем персонажа)
        PropertyIsCorrectionContinued,	//!< Блок с текстом ПРОД., вставляемый на обрыве реплики
        PropertyIsCorrectionCharacter,	//!< Блок с именем персонажа, вставляемый на новой странице
        PropertyIsBreakCorrectionStart,	//!< Разрывающий текст блок в начале разрыва
        PropertyIsBreakCorrectionEnd,	//!< Разрывающий текст блок в конце разрыва
        PropertyIsCharacterContinued,   //!< Имя персонажа для которого необходимо отображать допольнительный текст ПРОД., не пишем в xml
        //
        // Свойства форматирования
        //
        PropertyIsFormatting    //!< Пользовательский формат текста
    };

    /**
     * @brief Виды межстрочных интервалов
     */
    enum LineSpacing {
        SingleLineSpacing,
        OneAndHalfLineSpacing,
        DoubleLineSpacing,
        FixedLineSpacing
    };

    /**
     * @brief Получить тип блока
     */
    static ScreenplayParagraphType forBlock(const QTextBlock& _block);

public:
    ScreenplayBlockStyle() = default;

    /**
     * @brief Получить тип блока
     */
    ScreenplayParagraphType type() const;

    /**
     * @brief Получить активность стиля блока
     */
    bool isActive() const;

    /**
     * @brief Получить шрифт блока
     */
    QFont font() const;

    /**
     * @brief Выравнивание блока
     */
    Qt::Alignment align() const;

    /**
     * @brief Отступ сверху, линий
     */
    int topSpace() const;

    /**
     * @brief Отступ снизу, линий
     */
    int bottomSpace() const;

    /**
     * @brief Отступ слева, мм
     */
    qreal leftMargin() const;

    /**
     * @brief Отступ сверху, мм
     */
    qreal topMargin() const;

    /**
     * @brief Отступ справа, мм
     */
    qreal rightMargin() const;

    /**
     * @brief Отступ снизу, мм
     */
    qreal bottomMargin() const;

    /**
     * @brief Заданы ли отступы в блоке в мм [true], или в линиях [false]
     */
    bool hasVerticalSpacingInMM() const;

    /**
     * @brief Межстрочный интервал
     */
    LineSpacing lineSpacing() const;

    /**
     * @brief Значение межстрочного интервала для FixedLineSpacing, мм
     */
    qreal lineSpacingValue() const;

    /**
     * @brief Установить тип
     */
    void setType(ScreenplayParagraphType _type);

    /**
     * @brief Установить активность
     */
    void setIsActive(bool _isActive);

    /**
     * @brief Установить шрифт
     */
    void setFont(const QFont& _font);

    /**
     * @brief Установить выравнивание
     */
    void setAlign(Qt::Alignment _align);

    /**
     * @brief Установить отступ сверху, линий
     */
    void setTopSpace(int _topSpace);

    /**
     * @brief Установить отступ сверху, линий
     */
    void setBottomSpace(int _bottomSpace);

    /**
     * @brief Установить левый отступ, мм
     */
    void setLeftMargin(qreal _leftMargin);

    /**
     * @brief Установить верхний отступ, мм
     */
    void setTopMargin(qreal _topMargin);

    /**
     * @brief Установить правый отступ, мм
     */
    void setRightMargin(qreal _rightMargin);

    /**
     * @brief Установить нижний отступ, мм
     */
    void setBottomMargin(qreal _bottomMargin);

    /**
     * @brief Установить межстрочный интервал
     */
    void setLineSpacing(LineSpacing _lineSpacing);

    /**
     * @brief Значение межстрочного интервала для FixedLineSpacing, мм
     */
    void setLineSpacingValue(qreal _value);

    /**
     * @brief Настройки стиля отображения блока
     */
    QTextBlockFormat blockFormat() const;

    /**
     * @brief Установить цвет фона блока
     */
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Настройки шрифта блока
     */
    QTextCharFormat charFormat() const;

    /**
     * @brief Установить цвет текста
     */
    void setTextColor(const QColor& _color);

    /**
     * @brief Первый символ заглавный
     */
    bool isFirstUppercase() const;

    /**
     * @brief Разрешено изменять текст блока
     */
    bool isCanModify() const;
    void setCanModify(bool _can);

    /**
     * @brief Имеет ли стиль обрамление
     */
    bool hasDecoration() const;

    /**
     * @brief Префикс стиля
     */
    QString prefix() const;
    void setPrefix(const QString& _prefix);

    /**
     * @brief Постфикс стиля
     */
    QString postfix() const;
    void setPostfix(const QString& _postfix);

    /**
     * @brief Является ли блок частью группы
     */
    bool isEmbeddable() const;

    /**
     * @brief Является ли блок заголовком группы
     */
    bool isEmbeddableHeader() const;

    /**
     * @brief Блок закрывающий группу
     */
    ScreenplayParagraphType embeddableFooter() const;

private:
    /**
     * @brief Инициилизация возможна только в классе стиля сценария
     */
    explicit ScreenplayBlockStyle(const QXmlStreamAttributes& _blockAttributes);
    friend class ScreenplayTemplate;

    /**
     * @brief Обновить межстрочный интервал блока
     */
    void updateLineHeight();

    /**
     * @brief Обновить верхний отступ
     */
    void updateTopMargin();

    /**
     * @brief Обновить нижний отступ
     */
    void updateBottomMargin();

private:
    /**
     * @brief Тип блока
     */
    ScreenplayParagraphType m_type = ScreenplayParagraphType::Undefined;

    /**
     * @brief Активен ли блок
     */
    bool m_isActive = false;

    /**
     * @brief Шрифт блока
     */
    QFont m_font = QFont("Courier Prime", 12);

    /**
     * @brief Выравнивание блока
     */
    Qt::Alignment m_align = Qt::AlignLeft;

    /**
     * @brief Отступ сверху, линий
     */
    int m_topSpace = 0;

    /**
     * @brief Отступ снизу, линий
     */
    int m_bottomSpace = 0;

    /**
     * @brief Отступ слева, мм
     */
    qreal m_leftMargin = 0.0;

    /**
     * @brief Отступ сверху, мм
     */
    qreal m_topMargin = 0.0;

    /**
     * @brief Отступ справа, мм
     */
    qreal m_rightMargin = 0.0;

    /**
     * @brief Отступ снизу, мм
     */
    qreal m_bottomMargin = 0.0;

    /**
     * @brief Межстрочный интервал
     */
    LineSpacing m_lineSpacing = SingleLineSpacing;

    /**
     * @brief Значение межстрочного интервала для FixedLineSpacing, мм
     */
    qreal m_lineSpacingValue = 0.0;

    /**
     * @brief Формат блока
     */
    QTextBlockFormat m_blockFormat;

    /**
     * @brief Формат текста
     */
    QTextCharFormat m_charFormat;
};


/**
 * @brief Класс шаблона сценария
 */
class ScreenplayTemplate
{
public:
    ScreenplayTemplate() = default;

    /**
     * @brief Назначить шаблон новым
     */
    void setIsNew();

    /**
     * @brief Сохранить шаблон в файл
     */
    void saveToFile(const QString& _filePath) const;

    /**
     * @brief Является ли шаблон умолчальным
     */
    bool isDefault() const;

    /**
     * @brief Получить название
     */
    QString name() const;

    /**
     * @brief Получить описание
     */
    QString description() const;

    /**
     * @brief Получить размер страницы
     */
    QPageSize::PageSizeId pageSizeId() const;

    /**
     * @brief Получить отступы страницы в миллиметрах
     */
    QMarginsF pageMargins() const;

    /**
     * @brief Получить расположение нумерации
     */
    Qt::Alignment pageNumbersAlignment() const;

    /**
     * @brief Получить стиль блока заданного типа
     */
    ScreenplayBlockStyle blockStyle(ScreenplayParagraphType _forType) const;

    /**
     * @brief Получить стиль заданного блока
     */
    ScreenplayBlockStyle blockStyle(const QTextBlock& _forBlock) const;

    /**
     * @brief Установить наименование
     */
    void setName(const QString& _name);

    /**
     * @brief Установить описание
     */
    void setDescription(const QString& _description);

    /**
     * @brief Установить формат страницы
     */
    void setPageSizeId(QPageSize::PageSizeId _pageSizeId);

    /**
     * @brief Установить поля документа
     */
    void setPageMargins(const QMarginsF& _pageMargins);

    /**
     * @brief Установить расположение нумерации
     */
    void setNumberingAlignment(Qt::Alignment _alignment);

    /**
     * @brief Установить стиль блока
     */
    void setBlockStyle(const ScreenplayBlockStyle& _blockStyle);

    /**
     * @brief Обновить цвета прорисовки блоков
     */
    void updateBlocksColors();

private:
    explicit ScreenplayTemplate(const QString& _fromFile);
    friend class ScreenplayTemplateFacade;

    /**
     * @brief Загрузить шаблон из файла
     */
    void load(const QString& _fromFile);

private:
    /**
     * @brief Является ли шаблон умолчальным
     */
    bool m_isDefault = false;

    /**
     * @brief Название
     */
    QString m_name;

    /**
     * @brief Описание
     */
    QString m_description;

    /**
     * @brief Формат страницы
     */
    QPageSize::PageSizeId m_pageSizeId;

    /**
     * @brief Поля страницы в миллиметрах
     */
    QMarginsF m_pageMargins;

    /**
     * @brief Расположение нумерации
     */
    Qt::Alignment m_pageNumbersAlignment;

    /**
     * @brief Стили блоков текста
     */
    QMap<ScreenplayParagraphType, ScreenplayBlockStyle> m_blockStyles;
};

} // namespace BusinessLayer
