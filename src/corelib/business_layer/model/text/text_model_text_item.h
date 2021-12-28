#pragma once

#include "text_model_item.h"

#include <QTextLayout>

#include <optional>

class QXmlStreamReader;


namespace BusinessLayer {

enum class TextParagraphType;

/**
 * @brief Элемент модели текстового документа
 */
class CORE_LIBRARY_EXPORT TextModelTextItem : public TextModelItem
{
public:
    struct CORE_LIBRARY_EXPORT TextPart {
        int from = 0;
        int length = 0;
        int end() const;
    };
    struct CORE_LIBRARY_EXPORT TextFormat : TextPart {
        std::optional<QFont> font;
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

public:
    TextModelTextItem();
    explicit TextModelTextItem(QXmlStreamReader& _contentReaded);
    ~TextModelTextItem() override;

    /**
     * @brief Тип параграфа
     */
    TextParagraphType paragraphType() const;
    void setParagraphType(TextParagraphType _type);

    /**
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment() const;
    void setAlignment(Qt::Alignment _align);
    void clearAlignment();

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
    void setFormats(const QVector<QTextLayout::FormatRange>& _formats,
                    const QTextCharFormat& _blockCharFormat);

    /**
     * @brief Редакторские заметки
     */
    const QVector<ReviewMark>& reviewMarks() const;
    void setReviewMarks(const QVector<ReviewMark>& _reviewMarks);
    void setReviewMarks(const QVector<QTextLayout::FormatRange>& _reviewMarks);

    /**
     * @brief Объединить с заданным элементом
     */
    void mergeWith(const TextModelTextItem* _other);

    /**
     * @brief Определяем интерфейс получения данных элемента
     */
    QVariant data(int _role) const override;

    /**
     * @brief Сформировать xml блока
     */
    QByteArray toXml() const override;
    QByteArray toXml(int _from, int _length);

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelItem* _item) const override;

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
