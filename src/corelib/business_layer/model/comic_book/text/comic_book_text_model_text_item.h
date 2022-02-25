#pragma once

#include "comic_book_text_model_item.h"

#include <QColor>
#include <QString>
#include <QTextLayout>

#include <optional>

class QXmlStreamReader;


namespace BusinessLayer {

enum class ComicBookParagraphType;

/**
 * @brief Класс элемента текста модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelTextItem : public ComicBookTextModelItem
{
public:
    struct CORE_LIBRARY_EXPORT TextPart {
        int from = 0;
        int length = 0;
        int end() const;
    };
    struct CORE_LIBRARY_EXPORT TextFormat : TextPart {
        bool isBold = false;
        bool isItalic = false;
        bool isUnderline = false;
        bool isStrikethrough = false;

        bool operator==(const TextFormat& _other) const;

        bool isValid() const;

        QTextCharFormat charFormat() const;
    };
    struct CORE_LIBRARY_EXPORT ReviewComment {
        QString author;
        QString date;
        QString text;

        bool operator==(const ReviewComment& _other) const;
    };
    struct CORE_LIBRARY_EXPORT ReviewMark : TextPart {
        QColor textColor;
        QColor backgroundColor;
        bool isDone = false;
        QVector<ReviewComment> comments;

        bool operator==(const ReviewMark& _other) const;

        QTextCharFormat charFormat() const;
    };
    struct CORE_LIBRARY_EXPORT Number {
        QString value;
    };
    struct CORE_LIBRARY_EXPORT Bookmark {
        QColor color;
        QString text;

        bool operator==(const Bookmark& _other) const;
    };
    struct CORE_LIBRARY_EXPORT Revision : TextPart {
        QColor color;

        bool operator==(const Revision& _other) const;
    };

public:
    ComicBookTextModelTextItem();
    explicit ComicBookTextModelTextItem(QXmlStreamReader& _contentReaded);
    ~ComicBookTextModelTextItem() override;

    /**
     * @brief Номер элемента
     */
    std::optional<Number> number() const;
    void setNumber(int _number);

    /**
     * @brief Является ли блок декорацией
     */
    bool isCorrection() const;
    void setCorrection(bool _correction);

    /**
     * @brief Является ли блок корректировкой вида (CONT) внутри разорванной реплики
     */
    bool isCorrectionContinued() const;
    void setCorrectionContinued(bool _continued);

    /**
     * @brief Разорван ли текст блока между страницами
     */
    bool isBreakCorrectionStart() const;
    void setBreakCorrectionStart(bool _broken);
    bool isBreakCorrectionEnd() const;
    void setBreakCorrectionEnd(bool _broken);

    /**
     * @brief Находится ли элемент в первой колонке таблицы
     * @note Если значение не задано, то элемент находится вне таблицы
     */
    std::optional<bool> isInFirstColumn() const;
    void setInFirstColumn(const std::optional<bool>& _in);

    /**
     * @brief Тип параграфа
     */
    const ComicBookParagraphType& paragraphType() const;
    void setParagraphType(ComicBookParagraphType _type);

    /**
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment() const;
    void setAlignment(Qt::Alignment _align);
    void clearAlignment();

    /**
     * @brief Закладка
     */
    std::optional<Bookmark> bookmark() const;
    void setBookmark(const Bookmark& _bookmark);

    /**
     * @brief Текст элемента
     */
    const QString& text() const;
    void setText(const QString& _text);

    /**
     * @brief Удалить текст, начиная с заданной позиции, при этом корректируется и остальной контент
     * блока
     */
    void removeText(int _from);

    /**
     * @brief Форматирование в блоке
     */
    const QVector<TextFormat>& formats() const;
    void setFormats(const QVector<QTextLayout::FormatRange>& _formats);

    /**
     * @brief Редакторские заметки
     */
    const QVector<ReviewMark>& reviewMarks() const;
    void setReviewMarks(const QVector<ReviewMark>& _reviewMarks);
    void setReviewMarks(const QVector<QTextLayout::FormatRange>& _reviewMarks);

    /**
     * @brief Ревизии
     */
    const QVector<Revision>& revisions() const;

    /**
     * @brief Объединить с заданным элементом
     */
    void mergeWith(const ComicBookTextModelTextItem* _other);

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;
    QByteArray toXml(int _from, int _length);

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(ComicBookTextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(ComicBookTextModelItem* _item) const override;

private:
    /**
     * @brief Пометить блок изменённым
     */
    void markChanged();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


} // namespace BusinessLayer

Q_DECLARE_METATYPE(BusinessLayer::ComicBookTextModelTextItem::ReviewComment)
Q_DECLARE_METATYPE(QVector<BusinessLayer::ComicBookTextModelTextItem::ReviewComment>)
