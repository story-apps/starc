#pragma once

#include "screenplay_text_model_item.h"

#include <QColor>
#include <QString>
#include <QTextLayout>

class QDomElement;


namespace BusinessLayer
{

enum class ScreenplayParagraphType;

/**
 * @brief Класс элемента текста модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelTextItem : public ScreenplayTextModelItem
{
public:
    struct TextPart {
        int from = 0;
        int length = 0;
        int end() const;
    };
    struct TextFormat : TextPart {
        bool isBold = false;
        bool isItalic = false;
        bool isUnderline = false;

        bool operator==(const TextFormat& _other) const;

        bool isValid() const;
    };
    struct ReviewComment {
        QString author;
        QString date;
        QString text;

        bool operator==(const ReviewComment& _other) const;
    };
    struct ReviewMark : TextPart {
        QColor textColor;
        QColor backgroundColor;
        bool isDone = false;
        QVector<ReviewComment> comments;

        bool operator==(const ReviewMark& _other) const;

        QTextCharFormat charFormat() const;
    };
    struct Number {
        QString value;
    };

public:
    ScreenplayTextModelTextItem();
    explicit ScreenplayTextModelTextItem(const QDomElement& _node);
    ~ScreenplayTextModelTextItem() override;

    /**
     * @brief Является ли блок декорацией
     */
    bool isCorrection() const;
    void setCorrection(bool _correction);

    /**
     * @brief Тип параграфа
     */
    ScreenplayParagraphType paragraphType() const;
    void setParagraphType(ScreenplayParagraphType _type);

    /**
     * @brief Текст элемента
     */
    const QString& text() const;
    void setText(const QString& _text);

    /**
     * @brief Форматирование в блоке
     */
    void setFormats(const QVector<QTextLayout::FormatRange>& _formats);

    /**
     * @brief Редакторские заметки
     */
    const QVector<ReviewMark>& reviewMarks() const;
    void setReviewMarks(const QVector<ReviewMark>& _reviewMarks);
    void setReviewMarks(const QVector<QTextLayout::FormatRange>& _reviewMarks);

    /**
     * @brief Номер сцены
     */
    void setNumber(int _number);
    Number number() const;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QString toXml() const override;

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
