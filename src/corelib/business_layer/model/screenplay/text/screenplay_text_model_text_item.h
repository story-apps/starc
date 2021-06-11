#pragma once

#include "screenplay_text_model_item.h"

#include <QColor>
#include <QString>
#include <QTextLayout>

#include <optional>

class QXmlStreamReader;


namespace BusinessLayer {

enum class ScreenplayParagraphType;

/**
 * @brief Класс элемента текста модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelTextItem : public ScreenplayTextModelItem
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
    ScreenplayTextModelTextItem();
    explicit ScreenplayTextModelTextItem(QXmlStreamReader& _contentReaded);
    ~ScreenplayTextModelTextItem() override;

    /**
     * @brief Номер элемента
     */
    std::optional<Number> number() const;
    void setNumber(int _number);

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration() const;
    void updateDuration();

    /**
     * @brief Является ли блок декорацией
     */
    bool isCorrection() const;
    void setCorrection(bool _correction);

    /**
     * @brief Разорван ли текст блока между страницами
     */
    bool isBroken() const;
    void setBroken(bool _broken);

    /**
     * @brief Находится ли элемент в первой колонке таблицы
     * @note Если значение не задано, то элемент находится вне таблицы
     */
    std::optional<bool> isInFirstColumn() const;
    void setInFirstColumn(const std::optional<bool>& _in);

    /**
     * @brief Тип параграфа
     */
    ScreenplayParagraphType paragraphType() const;
    void setParagraphType(ScreenplayParagraphType _type);

    /**
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment() const;
    void setAlignment(Qt::Alignment _align);

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
    void mergeWith(const ScreenplayTextModelTextItem* _other);

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
    void copyFrom(ScreenplayTextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(ScreenplayTextModelItem* _item) const override;

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

Q_DECLARE_METATYPE(BusinessLayer::ScreenplayTextModelTextItem::ReviewComment)
Q_DECLARE_METATYPE(QVector<BusinessLayer::ScreenplayTextModelTextItem::ReviewComment>)
