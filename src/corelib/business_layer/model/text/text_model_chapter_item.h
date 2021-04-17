#pragma once

#include "text_model_item.h"

#include <QString>

class QXmlStreamReader;


namespace BusinessLayer
{

/**
 * @brief Класс элементов глав модели текста
 */
class CORE_LIBRARY_EXPORT TextModelChapterItem : public TextModelItem
{
public:
    /**
     * @brief Номер главы
     */
    struct Number {
        QString value;

        bool operator==(const Number& _other) const;
    };

    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        ChapterNumberRole = Qt::UserRole + 1,
        ChapterHeadingRole,
        ChapterTextRole,
        ChapterInlineNotesSizeRole,
        ChapterReviewMarksSizeRole,
        ChapterWordsCountRole
    };

public:
    TextModelChapterItem();
    explicit TextModelChapterItem(QXmlStreamReader& _contentReader);
    ~TextModelChapterItem() override;

    /**
     * @brief Номер главы
     */
    Number number() const;
    void setNumber(int _number);

    /**
     * @brief Количество слов главы
     */
    int wordsCount() const;

    /**
     * @brief Определяем интерфейс получения данных главы
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;
    QByteArray toXml(TextModelItem* _from, int _fromPosition, TextModelItem* _to, int _toPosition, bool _clearUuid) const;
    QByteArray xmlHeader(bool _clearUuid = false) const;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelItem* _item) const override;

protected:
    /**
     * @brief Обновляем текст главы при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
