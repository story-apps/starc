#pragma once

#include <QHash>
#include <QPageSize>
#include <QTextFormat>

#include <corelib_global.h>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer {

/**
 * @brief Типы параграфов в текстовом документ
 */
enum class TextParagraphType {
    Undefined,
    UnformattedText,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
    Text,
    InlineNote,
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(TextParagraphType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить текстовое представление типа блока
 */
CORE_LIBRARY_EXPORT QString toString(TextParagraphType _type);

/**
 * @brief Получить тип блока из текстового представления
 */
CORE_LIBRARY_EXPORT TextParagraphType textParagraphTypeFromString(const QString& _text);


/**
 * @brief Класс стиля блока текста
 */
class CORE_LIBRARY_EXPORT TextBlockStyle
{
public:
    /**
     * @brief Дополнительные свойства стилей текстовых блоков
     */
    enum Property {
        PropertyType = QTextFormat::UserProperty + 100, //!< Тип блока
        PropertyIsFirstUppercase, //!< Необходимо ли первый символ поднимать в верхний регистр
        //
        // Свойства редакторских заметок
        //
        PropertyIsReviewMark, //!< Формат является редакторской правкой
        PropertyIsHighlight, //!< Является ли правка аналогом выделения цветом из ворда
        PropertyIsDone, //!< Правка помечена как выполненная
        PropertyComments, //!< Список комментариев к правке
        PropertyCommentsAuthors, //!< Список авторов комментариев
        PropertyCommentsDates, //!< Список дат комментариев
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
    static TextParagraphType forBlock(const QTextBlock& _block);

public:
    TextBlockStyle() = default;

    /**
     * @brief Тип блока
     */
    TextParagraphType type() const;
    void setType(TextParagraphType _type);

    /**
     * @brief Активен ли стиль блока
     */
    bool isActive() const;
    void setActive(bool _isActive);

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
     * @brief Отступ снизу, линий
     */
    int linesAfter() const;
    void setLinesAfter(int _linesAfter);


    /**
     * @brief Настройки стиля отображения блока
     */
    QTextBlockFormat blockFormat() const;
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Настройки шрифта блока
     */
    QTextCharFormat charFormat() const;
    void setTextColor(const QColor& _color);

private:
    /**
     * @brief Инициилизация возможна только в классе стиля сценария
     */
    explicit TextBlockStyle(const QXmlStreamAttributes& _blockAttributes);
    friend class SimpleTextTemplate;

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
    TextParagraphType m_type = TextParagraphType::Undefined;

    /**
     * @brief Активен ли блок
     */
    bool m_isActive = false;

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
     * @brief Отступ снизу, линий
     */
    int m_linesAfter = 0;

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
 * @brief Класс шаблона текстового документа
 */
class CORE_LIBRARY_EXPORT SimpleTextTemplate
{
public:
    SimpleTextTemplate() = default;

    /**
     * @brief Назначить шаблон новым
     */
    void setIsNew();

    /**
     * @brief Сохранить шаблон в файл
     */
    void saveToFile(const QString& _filePath) const;

    /**
     * @brief Идентификатор шаблона
     */
    QString id() const;

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
     * @brief Получить стиль блока
     */
    TextBlockStyle paragraphStyle(TextParagraphType _forType) const;
    TextBlockStyle paragraphStyle(const QTextBlock& _forBlock) const;
    void setParagraphStyle(const TextBlockStyle& _style);

private:
    explicit SimpleTextTemplate(const QString& _fromFile);
    friend class TemplatesFacade;

    /**
     * @brief Загрузить шаблон из файла
     */
    void load(const QString& _fromFile);

private:
    /**
     * @brief Идентификатор
     */
    QString m_id;

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
    QHash<TextParagraphType, TextBlockStyle> m_blockStyles;
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(TextBlockStyle::LineSpacingType _type)
{
    return ::qHash(static_cast<int>(_type));
}

} // namespace BusinessLayer
