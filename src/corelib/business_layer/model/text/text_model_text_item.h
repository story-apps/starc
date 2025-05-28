#pragma once

#include "text_model_item.h"

#include <QColor>
#include <QString>
#include <QTextLayout>
#include <QUuid>

#include <optional>

class QXmlStreamReader;


namespace BusinessLayer {

enum class TextParagraphType;

/**
 * @brief Класс элемента текста модели текста
 */
class CORE_LIBRARY_EXPORT TextModelTextItem : public TextModelItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        TextNumberRole = Qt::UserRole + 1,
    };

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
        struct {
            QString family = {};
            int size = 0;
        } font;

        bool operator==(const TextFormat& _other) const;

        bool isValid() const;

        QTextCharFormat charFormat() const;
    };
    struct CORE_LIBRARY_EXPORT ReviewComment {
        QString author;
        QString authorEmail;
        QString date;
        QString text;
        bool isEdited = false;
        bool isRevision = false;
        bool isAddition = false;
        bool isRemoval = false;

        bool operator==(const ReviewComment& _other) const;

        /**
         * @brief Определить одинаков ли автор и дата создания коммента
         */
        bool isPartiallyEqual(const ReviewComment& _other) const;
    };
    struct CORE_LIBRARY_EXPORT ReviewMark : TextPart {
        QColor textColor;
        QColor backgroundColor;
        bool isDone = false;
        QVector<ReviewComment> comments;

        bool operator==(const ReviewMark& _other) const;

        /**
         * @brief Определить одинаково ли форматирование и первый комментарий
         */
        bool isPartiallyEqual(const ReviewMark& _other) const;

        QTextCharFormat charFormat() const;
    };
    struct CORE_LIBRARY_EXPORT ResourceMark : TextPart {
        QUuid uuid;

        bool operator==(const ResourceMark& _other) const;

        QTextCharFormat charFormat() const;
    };
    struct CORE_LIBRARY_EXPORT Number {
        int value = 0;
        QString text;
    };
    struct CORE_LIBRARY_EXPORT Bookmark {
        QColor color;
        QString name;

        bool operator==(const Bookmark& _other) const;

        bool isValid() const;
    };

public:
    explicit TextModelTextItem(const TextModel* _model);
    ~TextModelTextItem() override;

    /**
     * @brief Подтип элемента
     */
    int subtype() const override final;

    /**
     * @brief Тип параграфа
     */
    const TextParagraphType& paragraphType() const;
    void setParagraphType(TextParagraphType _type);

    /**
     * @brief Номер элемента
     */
    std::optional<Number> number() const;
    bool setNumber(int _number);

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
    void clearBookmark();

    /**
     * @brief Текст элемента
     */
    const QString& text() const;
    void setText(const QString& _text);

    /**
     * @brief Удалить текст, начиная с заданной позиции, при этом корректируется и остальной контент
     *        блока
     */
    void removeText(int _from, int _length = -1);

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
     * @brief Ресурсы блока
     */
    const QVector<ResourceMark>& resourceMarks() const;
    void setResourceMarks(const QVector<ResourceMark>& _resourceMarks);
    void setResourceMarks(const QVector<QTextLayout::FormatRange>& _formats);

    /**
     * @brief Объединить с заданным элементом
     */
    void mergeWith(const TextModelTextItem* _other);

    /**
     * @brief Определяем интерфейс получения данных блока
     */
    QVariant data(int _role) const override;

    /**
     * @brief Считать контент из заданного ридера
     */
    void readContent(QXmlStreamReader& _contentReader) override final;

    /**
     * @brief Определяем интерфейс для получения XML блока
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

    /**
     * @brief Обновить счётчики
     */
    virtual void updateCounters(bool _force = false);

protected:
    /**
     * @brief Пометить блок изменённым
     */
    void markChanged();

    /**
     * @brief Текст элемента, который будет сохранён
     * @note Иногда нужно сохранять тект не в таком виде, как он отображается при работе с ним,
     *       например текст диалогов комиксов сохраняется без номеров
     */
    virtual QString textToSave() const;

    /**
     * @brief Обновляем счетчики, при изменении текста
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


} // namespace BusinessLayer

Q_DECLARE_METATYPE(BusinessLayer::TextModelTextItem::ReviewComment)
Q_DECLARE_METATYPE(QVector<BusinessLayer::TextModelTextItem::ReviewComment>)
