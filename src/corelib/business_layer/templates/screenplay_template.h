#pragma once

#include <corelib_global.h>

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
    //
    PageSplitter, //!< Разделитель страницы (для блоков внутри которых находятся таблицы)
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(ScreenplayParagraphType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить текстовое представление типа блока
 */
CORE_LIBRARY_EXPORT QString toString(ScreenplayParagraphType _type);

/**
 * @brief Получить тип блока из текстового представления
 */
CORE_LIBRARY_EXPORT ScreenplayParagraphType screenplayParagraphTypeFromString(const QString& _text);


/**
 * @brief Класс стиля блока сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayBlockStyle
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
    enum class LineSpacingType {
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
     * @brief Тип блока
     */
    ScreenplayParagraphType type() const;
    void setType(ScreenplayParagraphType _type);

    /**
     * @brief Активен ли стиль блока
     */
    bool isActive() const;
    void setActive(bool _isActive);

    /**
     * @brief Экспортируется ли блок
     */
    bool isExportable() const;
    void setExportable(bool _isExportable);

    /**
     * @brief Располагается ли блок с начала страницы
     */
    bool isStartFromNewPage() const;
    void setStartFromNewPage(bool _startFromNewPage);

    /**
     * @brief Получить шрифт блока
     */
    QFont font() const;
    void setFont(const QFont& _font);

    /**
     * @brief Выравнивание блока
     */
    Qt::Alignment align() const;
    void setAlign(Qt::Alignment _align);

    /**
     * @brief Межстрочный интервал
     */
    LineSpacingType lineSpacingType() const;
    void setLineSpacingType(LineSpacingType _type);

    /**
     * @brief Значение межстрочного интервала для FixedLineSpacing, мм
     */
    qreal lineSpacingValue() const;
    void setLineSpacingValue(qreal _value);

    /**
     * @brief Отступ сверху, линий
     */
    int linesBefore() const;
    void setLinesBefore(int _linesBefore);

    /**
     * @brief Отступы вокруг блока, мм
     */
    QMarginsF margins() const;
    void setMargins(const QMarginsF& _margins);

    /**
     * @brief Отступы вокруг блока в режиме разделения на колонки, мм
     */
    QMarginsF marginsOnHalfPage() const;
    void setMarginsOnHalfPage(const QMarginsF& _margins);

    /**
     * @brief Отступ снизу, линий
     */
    int linesAfter() const;
    void setLinesAfter(int _linesAfter);


    /**
     * @brief Настройки стиля отображения блока
     */
    QTextBlockFormat blockFormat(bool _onHalfPage = false) const;
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Настройки шрифта блока
     */
    QTextCharFormat charFormat() const;
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
     * @brief Будет ли экспортироваться блок
     */
    bool m_isExportable = true;

    /**
     * @brief Начинается ли блок с начала страницы
     */
    bool m_isStartFromNewPage = false;

    /**
     * @brief Шрифт блока
     */
    QFont m_font = QFont("Courier Prime", 12);

    /**
     * @brief Выравнивание блока
     */
    Qt::Alignment m_align = Qt::AlignLeft;

    /**
     * @brief Межстрочный интервал
     */
    struct {
        LineSpacingType type = LineSpacingType::SingleLineSpacing;
        qreal value = 0.0;
    } m_lineSpacing;

    /**
     * @brief Отступ сверху, линий
     */
    int m_linesBefore = 0;

    /**
     * @brief Отступы вокруг блока, мм
     */
    QMarginsF m_margins;

    /**
     * @brief Отступы вокруг блока в режиме разделения на колонки, мм
     */
    QMarginsF m_marginsOnHalfPage;

    /**
     * @brief Отступ снизу, линий
     */
    int m_linesAfter = 0;

    /**
     * @brief Формат блока
     */
    QTextBlockFormat m_blockFormat;
    QTextBlockFormat m_blockFormatOnHalfPage;

    /**
     * @brief Формат текста
     */
    QTextCharFormat m_charFormat;
};


/**
 * @brief Класс шаблона сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTemplate
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
     * @brief Название
     */
    QString name() const;
    void setName(const QString& _name);

    /**
     * @brief Описание
     */
    QString description() const;
    void setDescription(const QString& _description);

    /**
     * @brief Размер страницы
     */
    QPageSize::PageSizeId pageSizeId() const;
    void setPageSizeId(QPageSize::PageSizeId _pageSizeId);

    /**
     * @brief Отступы страницы в миллиметрах
     */
    QMarginsF pageMargins() const;
    void setPageMargins(const QMarginsF& _pageMargins);

    /**
     * @brief Расположение нумерации
     */
    Qt::Alignment pageNumbersAlignment() const;
    void setPageNumbersAlignment(Qt::Alignment _alignment);

    /**
     * @brief Процент ширины страницы для левой части разделителя
     */
    int leftHalfOfPageWidthPercents() const;
    void setLeftHalfOfPageWidthPercents(int _width);

    /**
     * @brief Получить стиль блока
     */
    ScreenplayBlockStyle blockStyle(ScreenplayParagraphType _forType) const;
    ScreenplayBlockStyle blockStyle(const QTextBlock& _forBlock) const;
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
     * @brief Процент от ширины страницы, которые занимает левая часть разделения
     */
    int m_leftHalfOfPageWidthPercents = 50;

    /**
     * @brief Стили блоков текста
     */
    QHash<ScreenplayParagraphType, ScreenplayBlockStyle> m_blockStyles;
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
inline uint qHash(ScreenplayBlockStyle::LineSpacingType _type)
{
    return ::qHash(static_cast<int>(_type));
}

} // namespace BusinessLayer
