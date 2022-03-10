#pragma once

#include <QScopedPointer>

namespace BusinessLayer {

class TextModelItem;
class TextModelTextItem;

namespace xml {

/**
 * @brief Класс для формирования xml данных текстового документа
 * @note Основное предназначение - склеивать разорванные текстовые блоки перед записью
 */
class TextModelXmlWriter
{
public:
    explicit TextModelXmlWriter(bool _addHeader = false);
    ~TextModelXmlWriter();

    void operator+=(const char* _data);
    void operator+=(const QByteArray& _data);
    void operator+=(const QString& _data);
    void operator+=(TextModelItem* _item);
    struct TextItemData {
        TextModelTextItem* item = nullptr;
        int fromPosition = 0;
        int toPosition = 0;
    };
    void operator+=(const TextItemData& _data);

    QByteArray data() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace xml
} // namespace BusinessLayer
