#pragma once

#include <QHash>
#include <QPageSize>
#include <QTextFormat>

#include <corelib_global.h>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer {

/**
 * @brief Типы папок
 */
enum class TextFolderType {
    Undefined = 0,
    Act,
    Sequence,
};

/**
 * @brief Типы групп
 */
enum class TextGroupType {
    Undefined = 10,
    Scene,
    Beat,
    Page,
    Panel,
    Chapter,
};

/**
 * @brief Типы параграфов
 */
enum class TextParagraphType {
    //
    // Общие для всех стилей
    //
    Undefined = 100,
    UnformattedText, //!< Простой текст (без особенного форматирования и отступов)
    InlineNote, //!< Заметка по тексту
    ActHeading, //!< Заголовок акта
    ActFooter, //!< Окончание акта
    SequenceHeading, //!< Заголовок папки
    SequenceFooter, //!< Окончание папки
    //
    PageSplitter, //!< Разделитель страницы (для блоков внутри которых находятся таблицы)
    //
    // Скрипты
    //
    SceneHeading, //!< Заголовок сцены
    SceneHeadingShadow, //!< Заголовок сцены, для вспомогательных разрывов
    Character, //!< Имя персонажа
    Dialogue, //!< Реплика персонажа
    //
    // ... киносценарий и частично пьесы
    //
    SceneCharacters, //!< Персонажи сцены
    BeatHeading, //!< Бит истории
    BeatHeadingShadow, //!< Бит истории, для вспомогательных разрывов
    Action, //!< Описание действия
    Parenthetical, //!< Ремарка в реплике персонажа
    Lyrics, //!< Песнь или стихотворения произносимые персонажем
    Transition, //!< Переход между кадрами
    Shot, //!< Кадр
    //
    // ... радиопостановка
    //
    Sound, //!< Звуковой эффект
    Music, //!< Музыка
    Cue, //!< Спецсигнал для актёра
    //
    // Комикс
    //
    PageHeading, //!< Страница
    PanelHeading, //!< Панель
    PanelHeadingShadow, //!< Панель, для вспомогательных разрывов
    Description, //!< Описание поисходящего на панели
    //
    // Простой текст
    //
    ChapterHeading1, //!< Заголовок 1
    ChapterHeading2, //!< Заголовок 2
    ChapterHeading3, //!< Заголовок 3
    ChapterHeading4, //!< Заголовок 4
    ChapterHeading5, //!< Заголовок 5
    ChapterHeading6, //!< Заголовок 6
    Text, //!< Текст главы
};

/**
 * @brief Определим методы для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(TextFolderType _type)
{
    return ::qHash(static_cast<int>(_type));
}
CORE_LIBRARY_EXPORT inline uint qHash(TextGroupType _type)
{
    return ::qHash(static_cast<int>(_type));
}
CORE_LIBRARY_EXPORT inline uint qHash(TextParagraphType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить текстовое представление типа папки
 */
CORE_LIBRARY_EXPORT QString toString(TextFolderType _type);

/**
 * @brief Получить тип папки из текстового представления
 */
CORE_LIBRARY_EXPORT TextFolderType textFolderTypeFromString(const QString& _text);

/**
 * @brief Получить текстовое представление типа группы
 */
CORE_LIBRARY_EXPORT QString toString(TextGroupType _type);

/**
 * @brief Получить тип группы из текстового представления
 */
CORE_LIBRARY_EXPORT TextGroupType textGroupTypeFromString(const QString& _text);

/**
 * @brief Получить текстовое представление типа блока
 */
CORE_LIBRARY_EXPORT QString toString(TextParagraphType _type);
CORE_LIBRARY_EXPORT QString toDisplayString(TextParagraphType _type);

/**
 * @brief Получить тип блока из текстового представления
 */
CORE_LIBRARY_EXPORT TextParagraphType textParagraphTypeFromString(const QString& _text);
CORE_LIBRARY_EXPORT TextParagraphType textParagraphTypeFromDisplayString(const QString& _text);

/**
 * @brief Тайтл элемента заданного типа
 */
CORE_LIBRARY_EXPORT QString textParagraphTitle(TextParagraphType _type);


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
        PropertyHeaderType, //!< Тип блока заголовка
        PropertyPrefix, //!< Префикс блока
        PropertyPostfix, //!< Постфикс блока
        PropertyIsFirstUppercase, //!< Необходимо ли первый символ поднимать в верхний регистр
        PropertyIsCanModify, //!< Редактируемый ли блок
        //
        // Свойства редакторских заметок
        //
        PropertyIsReviewMark, //!< Формат является редакторской правкой
        PropertyIsDone, //!< Правка помечена как выполненная
        PropertyComments, //!< Список комментариев к правке
        PropertyCommentsAuthors, //!< Список авторов комментариев
        PropertyCommentsDates, //!< Список дат комментариев
        PropertyCommentsIsEdited, //!< Список признаков изменений комментариев
        //
        // Свойства корректирующих текст блоков
        //
        PropertyIsCorrection, //!< Не разрывающий текст блок (пустые блоки в конце страницы, блоки с
                              //!< текстом ПРОД, или именем персонажа)
        PropertyIsCorrectionContinued, //!< Блок с текстом ПРОД., вставляемый на обрыве реплики
        PropertyIsCorrectionCharacter, //!< Блок с именем персонажа, вставляемый на новой странице
        PropertyIsBreakCorrectionStart, //!< Разрывающий текст блок в начале разрыва
        PropertyIsBreakCorrectionEnd, //!< Разрывающий текст блок в конце разрыва
        PropertyIsCharacterContinued, //!< Имя персонажа для которого необходимо отображать
                                      //!< допольнительный текст ПРОД., не пишем в xml
    };

    /**
     * @brief Виды межстрочных интервалов
     */
    enum class LineSpacingType {
        SingleLineSpacing,
        OneAndHalfLineSpacing,
        DoubleLineSpacing,
        FixedLineSpacing,
    };

    /**
     * @brief Получить тип параграфа для заданного блока
     */
    static TextParagraphType forBlock(const QTextCursor& _cursor);
    static TextParagraphType forBlock(const QTextBlock& _block);

public:
    TextBlockStyle() = default;

    /**
     * @brief Тип параграфа
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
     * @brief Отступы вокруг блока в режиме разделения на колонки, мм
     */
    QMarginsF marginsOnHalfPage() const;
    void setMarginsOnHalfPage(const QMarginsF& _margins);

    /**
     * @brief Настроить стиль в соответствии с шириной разделителя страницы
     */
    void setPageSplitterWidth(qreal _width);

    /**
     * @brief Отступ снизу, линий
     */
    int linesAfter() const;
    void setLinesAfter(int _linesAfter);

    /**
     * @brief Отображать ли загаловок блока
     */
    bool isTitleVisible() const;
    void setTitleVisible(bool _visible);

    /**
     * @brief Кастомный заголовок блока
     */
    QString title() const;
    void setTitle(const QString& _title);


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
     * @brief Разрешено изменять текст блока
     */
    bool isCanModify() const;

    /**
     * @brief Префикс стиля
     */
    QString prefix() const;

    /**
     * @brief Постфикс стиля
     */
    QString postfix() const;

private:
    /**
     * @brief Инициилизация возможна только в классе шаблона
     */
    explicit TextBlockStyle(const QXmlStreamAttributes& _blockAttributes);
    friend class TextTemplate;

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
     * @brief Отступы вокруг блока в режиме разделения на колонки, мм
     */
    QMarginsF m_marginsOnHalfPage;

    /**
     * @brief Отступ снизу, линий
     */
    int m_linesAfter = 0;

    /**
     * @brief Отображать ли заголовок блока
     */
    bool m_isTitleVisible = false;

    /**
     * @brief Кастомный заголовок блока. Если не задан, то используется дефолтный
     */
    QString m_title;

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
 * @brief Класс шаблона оформления текста
 */
class CORE_LIBRARY_EXPORT TextTemplate
{
public:
    TextTemplate();
    explicit TextTemplate(const QString& _fromFile);
    TextTemplate(const TextTemplate& _other);
    TextTemplate& operator=(const TextTemplate& _other);
    ~TextTemplate();

    /**
     * @brief Загрузить шаблон из файла
     */
    void load(const QString& _fromFile);

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
     * @brief Валиден ли шаблон
     */
    bool isValid() const;

    /**
     * @brief Является ли шаблон умолчальным
     */
    bool isDefault() const;

    /**
     * @brief Название
     */
    virtual QString name() const;
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
     * @brief Ширина разделителя колонок
     */
    qreal pageSplitterWidth() const;

    /**
     * @brief Можно ли обхединять таблицы в данном шаблоне
     */
    virtual bool canMergeParagraph() const;

    /**
     * @brief Дефолтный шрифт шаблона
     */
    QFont defaultFont() const;

    /**
     * @brief Шаблон оформления титульной страницы
     */
    const TextTemplate& titlePageTemplate() const;

    /**
     * @brief Стандартный текст титульной страницы
     */
    const QString& titlePage() const;
    void setTitlePage(const QString& _titlePage);

    /**
     * @brief Шаблон оформления синопсиса
     */
    const TextTemplate& synopsisTemplate() const;

    /**
     * @brief Получить стиль блока
     */
    TextBlockStyle paragraphStyle(TextParagraphType _forType) const;
    TextBlockStyle paragraphStyle(const QTextBlock& _forBlock) const;
    void setParagraphStyle(const TextBlockStyle& _style);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(TextBlockStyle::LineSpacingType _type)
{
    return ::qHash(static_cast<int>(_type));
}

} // namespace BusinessLayer
