#pragma once

#include <corelib_global.h>

#include <business_layer/model/abstract_model_item.h>

#include <QTextLayout>

class QXmlStreamReader;


namespace BusinessLayer
{

/**
 * @brief Элемент модели текстового документа
 */
class CORE_LIBRARY_EXPORT TextModelTextItem : public AbstractModelItem
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

public:
    TextModelTextItem();
    explicit TextModelTextItem(QXmlStreamReader& _contentReaded);
    ~TextModelTextItem() override;

    /**
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment() const;
    void setAlignment(Qt::Alignment _align);

    /**
     * @brief Текст элемента
     */
    const QString& text() const;
    void setText(const QString& _text);

    /**
     * @brief Удалить текст, начиная с заданной позиции, при этом корректируется и остальной контент блока
     */
    void removeText(int _from);

    /**
     * @brief Форматирование в блоке
     */
    const QVector<TextFormat>& formats() const;
    void setFormats(const QVector<QTextLayout::FormatRange>& _formats);

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
    QByteArray toXml() const;
    QByteArray toXml(int _from, int _length);

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelTextItem* _item);

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelTextItem* _item) const;

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
